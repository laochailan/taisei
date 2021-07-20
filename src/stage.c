/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "util.h"
#include "stage.h"
#include "global.h"
#include "video.h"
#include "resource/bgm.h"
#include "replay/state.h"
#include "replay/stage.h"
#include "replay/struct.h"
#include "config.h"
#include "player.h"
#include "menu/ingamemenu.h"
#include "menu/gameovermenu.h"
#include "audio/audio.h"
#include "log.h"
#include "stagetext.h"
#include "stagedraw.h"
#include "stageobjects.h"
#include "eventloop/eventloop.h"
#include "common_tasks.h"
#include "stageinfo.h"

typedef struct StageFrameState {
	StageInfo *stage;
	CallChain cc;
	CoSched sched;
	int transition_delay;
	int logic_calls;
	int desync_check_freq;
	uint16_t last_replay_fps;
	float view_shake;
} StageFrameState;

static StageFrameState *_current_stage_state;  // TODO remove this shitty hack

#define BGM_FADE_LONG (2.0 * FADE_TIME / (double)FPS)
#define BGM_FADE_SHORT (FADE_TIME / (double)FPS)

#ifdef HAVE_SKIP_MODE

static struct {
	const char *skip_to_bookmark;
	bool skip_to_dialog;
	bool was_skip_mode;
	int bgm_start_time;
	double bgm_start_pos;
} skip_state;

void _stage_bookmark(const char *name) {
	log_debug("Bookmark [%s] reached at %i", name, global.frames);

	if(skip_state.skip_to_bookmark && !strcmp(skip_state.skip_to_bookmark, name)) {
		skip_state.skip_to_bookmark = NULL;
		global.plr.iddqd = false;
	}
}

DEFINE_EXTERN_TASK(stage_bookmark) {
	_stage_bookmark(ARGS.name);
}

bool stage_is_skip_mode(void) {
	return skip_state.skip_to_bookmark || skip_state.skip_to_dialog;
}

static void skipstate_init(void) {
	skip_state.skip_to_dialog = env_get_int("TAISEI_SKIP_TO_DIALOG", 0);
	skip_state.skip_to_bookmark = env_get_string_nonempty("TAISEI_SKIP_TO_BOOKMARK", NULL);
}

static LogicFrameAction skipstate_handle_frame(void) {
	if(skip_state.skip_to_dialog && dialog_is_active(global.dialog)) {
		skip_state.skip_to_dialog = false;
		global.plr.iddqd = false;
	}

	bool skip_mode = stage_is_skip_mode();

	if(!skip_mode && skip_state.was_skip_mode) {
		audio_bgm_seek_realtime(skip_state.bgm_start_pos + (global.frames - skip_state.bgm_start_time) / (double)FPS);
	}

	skip_state.was_skip_mode = skip_mode;

	if(skip_mode) {
		return LFRAME_SKIP_ALWAYS;
	}

	if(gamekeypressed(KEY_SKIP)) {
		return LFRAME_SKIP;
	}

	return LFRAME_WAIT;
}

static void skipstate_shutdown(void) {
	memset(&skip_state, 0, sizeof(skip_state));
}

static void skipstate_handle_bgm_change(void) {
	skip_state.bgm_start_time = global.frames;
	skip_state.bgm_start_pos = audio_bgm_tell();
}

#else

INLINE LogicFrameAction skipstate_handle_frame(void) { return LFRAME_WAIT; }
INLINE void skipstate_handle_bgm_change(void) { }
INLINE void skipstate_init(void) { }
INLINE void skipstate_shutdown(void) { }

#endif

static void stage_start(StageInfo *stage) {
	global.timer = 0;
	global.frames = 0;
	global.gameover = 0;
	global.voltage_threshold = 0;

	player_stage_pre_init(&global.plr);

	stats_stage_reset(&global.plr.stats);

	if(stage->type == STAGE_SPELL) {
		global.is_practice_mode = true;
		global.plr.lives = 0;
		global.plr.bombs = 0;
	} else if(global.is_practice_mode) {
		global.plr.lives = PLR_STGPRACTICE_LIVES;
		global.plr.bombs = PLR_STGPRACTICE_BOMBS;
	}

	if(global.is_practice_mode) {
		global.plr.power = config_get_int(CONFIG_PRACTICE_POWER);
	}

	if(global.plr.power < 0) {
		global.plr.power = 0;
	} else if(global.plr.power > PLR_MAX_POWER) {
		global.plr.power = PLR_MAX_POWER;
	}

	reset_all_sfx();
}

static bool ingame_menu_interrupts_bgm(void) {
	return global.stage->type != STAGE_SPELL;
}

typedef struct IngameMenuContext {
	CallChain next;
	BGM *saved_bgm;
	double saved_bgm_pos;
	bool bgm_interrupted;
} IngameMenuContext;

static void setup_ingame_menu_bgm(IngameMenuContext *ctx, BGM *bgm) {
	if(ingame_menu_interrupts_bgm()) {
		ctx->bgm_interrupted = true;

		if(bgm) {
			ctx->saved_bgm = audio_bgm_current();
			ctx->saved_bgm_pos = audio_bgm_tell();
			audio_bgm_play(bgm, true, 0, 1);
		} else {
			audio_bgm_pause();
		}
	}
}

static void resume_bgm(IngameMenuContext *ctx) {
	if(ctx->bgm_interrupted) {
		if(ctx->saved_bgm) {
			audio_bgm_play(ctx->saved_bgm, true, ctx->saved_bgm_pos, 0.5);
			ctx->saved_bgm = NULL;
		} else {
			audio_bgm_resume();
		}

		ctx->bgm_interrupted = false;
	}
}

static void stage_leave_ingame_menu(CallChainResult ccr) {
	IngameMenuContext *ctx = ccr.ctx;
	MenuData *m = ccr.result;

	if(m->state != MS_Dead) {
		return;
	}

	if(global.gameover > 0) {
		stop_all_sfx();

		if(ctx->bgm_interrupted) {
			audio_bgm_stop(global.gameover == GAMEOVER_RESTART ? BGM_FADE_SHORT : BGM_FADE_LONG);
		}
	} else {
		resume_bgm(ctx);
	}

	resume_all_sfx();

	run_call_chain(&ctx->next, NULL);
	free(ctx);
}

static void stage_enter_ingame_menu(MenuData *m, BGM *bgm, CallChain next) {
	IngameMenuContext *ctx = calloc(1, sizeof(*ctx));
	ctx->next = next;
	setup_ingame_menu_bgm(ctx, bgm);
	pause_all_sfx();
	enter_menu(m, CALLCHAIN(stage_leave_ingame_menu, ctx));
}

void stage_pause(void) {
	if(global.gameover == GAMEOVER_TRANSITIONING || stage_is_skip_mode()) {
		return;
	}

	MenuData *m;

	if(global.replay.input.replay) {
		m = create_ingame_menu_replay();
	} else {
		m = create_ingame_menu();
	}

	stage_enter_ingame_menu(m, NULL, NO_CALLCHAIN);
}

void stage_gameover(void) {
	if(global.stage->type == STAGE_SPELL && config_get_int(CONFIG_SPELLSTAGE_AUTORESTART)) {
		global.gameover = GAMEOVER_RESTART;
		return;
	}

	BGM *bgm = NULL;

	if(ingame_menu_interrupts_bgm()) {
		bgm = res_bgm("gameover");
		progress_unlock_bgm("gameover");
	}

	stage_enter_ingame_menu(create_gameover_menu(), bgm, NO_CALLCHAIN);
}

static bool stage_input_common(SDL_Event *event, void *arg) {
	TaiseiEvent type = TAISEI_EVENT(event->type);
	int32_t code = event->user.code;

	switch(type) {
		case TE_GAME_KEY_DOWN:
			switch(code) {
				case KEY_STOP:
					stage_finish(GAMEOVER_DEFEAT);
					return true;

				case KEY_RESTART:
					stage_finish(GAMEOVER_RESTART);
					return true;
			}

			break;

		case TE_GAME_PAUSE:
			stage_pause();
			break;

		default:
			break;
	}

	return false;
}

static bool stage_input_key_filter(KeyIndex key, bool is_release) {
	if(key == KEY_HAHAIWIN) {
		IF_DEBUG(
			if(!is_release) {
				stage_finish(GAMEOVER_WIN);
			}
		);

		return false;
	}

	IF_NOT_DEBUG(
		if(
			key == KEY_IDDQD ||
			key == KEY_POWERUP ||
			key == KEY_POWERDOWN
		) {
			return false;
		}
	);

	if(stage_is_cleared()) {
		if(key == KEY_SHOT) {
			if(
				global.gameover == GAMEOVER_SCORESCREEN &&
				global.frames - global.gameover_time > GAMEOVER_SCORE_DELAY * 2
			) {
				if(!is_release) {
					stage_finish(GAMEOVER_WIN);
				}
			}
		}

		if(key == KEY_BOMB || key == KEY_SPECIAL) {
			return false;
		}
	}

	return true;
}

static bool stage_input_handler_gameplay(SDL_Event *event, void *arg) {
	TaiseiEvent type = TAISEI_EVENT(event->type);
	int32_t code = event->user.code;

	if(stage_input_common(event, arg)) {
		return false;
	}

	if(
		(type == TE_GAME_KEY_DOWN || type == TE_GAME_KEY_UP) &&
		code == KEY_SHOT &&
		config_get_int(CONFIG_SHOT_INVERTED)
	) {
		type = type == TE_GAME_KEY_DOWN ? TE_GAME_KEY_UP : TE_GAME_KEY_DOWN;
	}

	ReplayState *rpy = &global.replay.output;

	switch(type) {
		case TE_GAME_KEY_DOWN:
			if(stage_input_key_filter(code, false)) {
				player_event(&global.plr, NULL, rpy, EV_PRESS, code);
			}
			break;

		case TE_GAME_KEY_UP:
			if(stage_input_key_filter(code, true)) {
				player_event(&global.plr, NULL, rpy, EV_RELEASE, code);
			}
			break;

		case TE_GAME_AXIS_LR:
			player_event(&global.plr, NULL, rpy, EV_AXIS_LR, (uint16_t)code);
			break;

		case TE_GAME_AXIS_UD:
			player_event(&global.plr, NULL, rpy, EV_AXIS_UD, (uint16_t)code);
			break;

		default: break;
	}

	return false;
}

static bool stage_input_handler_replay(SDL_Event *event, void *arg) {
	stage_input_common(event, arg);
	return false;
}

static void handle_replay_event(ReplayEvent *e, void *arg) {
	ReplayState *st = NOT_NULL(arg);

	if(e->type == EV_OVER) {
		global.gameover = GAMEOVER_DEFEAT;
	} else {
		player_event(&global.plr, st, &global.replay.output, e->type, e->value);
	}
}

static void replay_input(void) {
	events_poll((EventHandler[]){
		{ .proc = stage_input_handler_replay },
		{ NULL }
	}, EFLAG_GAME);

	ReplayState *st = &global.replay.input;
	replay_state_play_advance(st, global.frames, handle_replay_event, st);
	player_applymovement(&global.plr);
}

static void display_bgm_title(void) {
	BGM *bgm = audio_bgm_current();
	const char *title = bgm ? bgm_get_title(bgm) : NULL;

	if(title) {
		char txt[strlen(title) + 6];
		snprintf(txt, sizeof(txt), "BGM: %s", title);
		stagetext_add(txt, VIEWPORT_W-15 + I * (VIEWPORT_H-20), ALIGN_RIGHT, res_font("standard"), RGB(1, 1, 1), 30, 180, 35, 35);
	}
}

static bool stage_handle_bgm_change(SDL_Event *evt, void *a) {
	if(dialog_is_active(global.dialog)) {
		INVOKE_TASK_WHEN(&global.dialog->events.fadeout_began, common_call_func, display_bgm_title);
	} else {
		display_bgm_title();
	}

	skipstate_handle_bgm_change();
	return false;
}

static void stage_input(void) {
	if(stage_is_skip_mode()) {
		events_poll((EventHandler[]){
			{ .proc = stage_handle_bgm_change, .event_type = MAKE_TAISEI_EVENT(TE_AUDIO_BGM_STARTED) },
			{NULL}
		}, EFLAG_NOPUMP);
	} else {
		events_poll((EventHandler[]){
			{ .proc = stage_input_handler_gameplay },
			{ .proc = stage_handle_bgm_change, .event_type = MAKE_TAISEI_EVENT(TE_AUDIO_BGM_STARTED) },
			{NULL}
		}, EFLAG_GAME);
	}

	player_fix_input(&global.plr, &global.replay.output);
	player_applymovement(&global.plr);
}

static void stage_logic(void) {
	process_boss(&global.boss);
	process_enemies(&global.enemies);
	process_projectiles(&global.projs, true);
	process_items();
	process_lasers();
	process_projectiles(&global.particles, false);

	if(global.dialog) {
		dialog_update(global.dialog);

		if((global.plr.inputflags & INFLAG_SKIP) && dialog_is_active(global.dialog)) {
			dialog_page(global.dialog);
		}
	}

	if(stage_is_skip_mode()) {
		if(dialog_is_active(global.dialog)) {
			dialog_page(global.dialog);
		}

		if(global.boss) {
			ent_damage(&global.boss->ent, &(DamageInfo) { 400, DMG_PLAYER_SHOT } );
		}
	}

	update_all_sfx();

	global.frames++;

	if(!dialog_is_active(global.dialog) && (!global.boss || boss_is_fleeing(global.boss))) {
		global.timer++;
	}

	if(global.replay.input.replay && global.gameover != GAMEOVER_TRANSITIONING) {
		ReplayStage *rstg = global.replay.input.stage;
		ReplayEvent *last_event = dynarray_get_ptr(&rstg->events, rstg->events.num_elements - 1);

		if(global.frames == last_event->frame - FADE_TIME) {
			stage_finish(GAMEOVER_DEFEAT);
		}
	}

	stagetext_update();
}

void stage_clear_hazards_predicate(bool (*predicate)(EntityInterface *ent, void *arg), void *arg, ClearHazardsFlags flags) {
	bool force = flags & CLEAR_HAZARDS_FORCE;

	if(flags & CLEAR_HAZARDS_BULLETS) {
		for(Projectile *p = global.projs.first, *next; p; p = next) {
			next = p->next;

			if(!force && !projectile_is_clearable(p)) {
				continue;
			}

			if(!predicate || predicate(&p->ent, arg)) {
				clear_projectile(p, flags);
			}
		}
	}

	if(flags & CLEAR_HAZARDS_LASERS) {
		for(Laser *l = global.lasers.first, *next; l; l = next) {
			next = l->next;

			if(!force && !laser_is_clearable(l)) {
				continue;
			}

			if(!predicate || predicate(&l->ent, arg)) {
				clear_laser(l, flags);
			}
		}
	}
}

void stage_clear_hazards(ClearHazardsFlags flags) {
	stage_clear_hazards_predicate(NULL, NULL, flags);
}

static bool proximity_predicate(EntityInterface *ent, void *varg) {
	Circle *area = varg;

	switch(ent->type) {
		case ENT_TYPE_ID(Projectile): {
			Projectile *p = ENT_CAST(ent, Projectile);
			return cabs(p->pos - area->origin) < area->radius;
		}

		case ENT_TYPE_ID(Laser): {
			Laser *l = ENT_CAST(ent, Laser);
			return laser_intersects_circle(l, *area);
		}

		default: UNREACHABLE;
	}
}

static bool ellipse_predicate(EntityInterface *ent, void *varg) {
	Ellipse *e = varg;

	switch(ent->type) {
		case ENT_TYPE_ID(Projectile): {
			Projectile *p = ENT_CAST(ent, Projectile);
			return point_in_ellipse(p->pos, *e);
		}

		case ENT_TYPE_ID(Laser): {
			Laser *l = ENT_CAST(ent, Laser);
			return laser_intersects_ellipse(l, *e);
		}

		default: UNREACHABLE;
	}
}

void stage_clear_hazards_at(cmplx origin, double radius, ClearHazardsFlags flags) {
	Circle area = { origin, radius };
	stage_clear_hazards_predicate(proximity_predicate, &area, flags);
}

void stage_clear_hazards_in_ellipse(Ellipse e, ClearHazardsFlags flags) {
	stage_clear_hazards_predicate(ellipse_predicate, &e, flags);
}

TASK(clear_dialog, NO_ARGS) {
	assert(global.dialog != NULL);
	// dialog_deinit() should've been called by dialog_end() at this point
	global.dialog = NULL;
}

TASK(dialog_fixup_timer, NO_ARGS) {
	// HACK: remove when global.timer is gone
	global.timer++;
}

void stage_begin_dialog(Dialog *d) {
	assert(global.dialog == NULL);
	assert(!global.gameover);
	global.dialog = d;
	dialog_init(d);
	INVOKE_TASK_WHEN(&d->events.fadeout_began, dialog_fixup_timer);
	INVOKE_TASK_WHEN(&d->events.fadeout_ended, clear_dialog);
}

static void stage_free(void) {
	delete_enemies(&global.enemies);
	delete_items();
	delete_lasers();

	delete_projectiles(&global.projs);
	delete_projectiles(&global.particles);

	if(global.dialog) {
		dialog_deinit(global.dialog);
		global.dialog = NULL;
	}

	if(global.boss) {
		free_boss(global.boss);
		global.boss = NULL;
	}

	projectiles_free();
	lasers_free();
	stagetext_free();
}

static void stage_finalize(void *arg) {
	global.gameover = (intptr_t)arg;
}

void stage_finish(int gameover) {
	if(global.gameover == GAMEOVER_TRANSITIONING) {
		return;
	}

	int prev_gameover = global.gameover;
	global.gameover_time = global.frames;

	if(gameover == GAMEOVER_SCORESCREEN) {
		global.gameover = GAMEOVER_SCORESCREEN;
	} else {
		global.gameover = GAMEOVER_TRANSITIONING;
		set_transition_callback(TransFadeBlack, FADE_TIME, FADE_TIME*2, stage_finalize, (void*)(intptr_t)gameover);
		audio_bgm_stop(BGM_FADE_LONG);
	}

	if(
		global.replay.input.replay == NULL &&
		prev_gameover != GAMEOVER_SCORESCREEN &&
		(gameover == GAMEOVER_SCORESCREEN || gameover == GAMEOVER_WIN)
	) {
		StageProgress *p = stageinfo_get_progress(global.stage, global.diff, true);

		if(p) {
			++p->num_cleared;
			log_debug("Stage cleared %u times now", p->num_cleared);
		}
	}

	if(gameover == GAMEOVER_SCORESCREEN && stage_is_skip_mode()) {
		// don't get stuck in an infinite loop
		taisei_quit();
	}
}

static void stage_preload(void) {
	difficulty_preload();
	projectiles_preload();
	player_preload();
	items_preload();
	boss_preload();
	lasers_preload();
	enemies_preload();

	if(global.stage->type != STAGE_SPELL) {
		preload_resource(RES_BGM, "gameover", RESF_DEFAULT);
		dialog_preload();
	}

	global.stage->procs->preload();
}

static void display_stage_title(StageInfo *info) {
	stagetext_add(info->title,    VIEWPORT_W/2 + I * (VIEWPORT_H/2-40), ALIGN_CENTER, res_font("big"), RGB(1, 1, 1), 50, 85, 35, 35);
	stagetext_add(info->subtitle, VIEWPORT_W/2 + I * (VIEWPORT_H/2),    ALIGN_CENTER, res_font("standard"), RGB(1, 1, 1), 60, 85, 35, 35);
}

TASK(start_bgm, { BGM *bgm; }) {
	audio_bgm_play(ARGS.bgm, true, 0, 0);
}

void stage_start_bgm(const char *bgm) {
	INVOKE_TASK_DELAYED(1, start_bgm, res_bgm(bgm));
}

void stage_set_voltage_thresholds(uint easy, uint normal, uint hard, uint lunatic) {
	switch(global.diff) {
		case D_Easy:    global.voltage_threshold = easy;    return;
		case D_Normal:  global.voltage_threshold = normal;  return;
		case D_Hard:    global.voltage_threshold = hard;    return;
		case D_Lunatic: global.voltage_threshold = lunatic; return;
		default: UNREACHABLE;
	}
}

bool stage_is_cleared(void) {
	return global.gameover == GAMEOVER_SCORESCREEN || global.gameover == GAMEOVER_TRANSITIONING;
}

static void stage_update_fps(StageFrameState *fstate) {
	if(global.replay.output.replay) {
		uint16_t replay_fps = (uint16_t)rint(global.fps.logic.fps);

		if(replay_fps != fstate->last_replay_fps) {
			replay_stage_event(global.replay.output.stage, global.frames, EV_FPS, replay_fps);
			fstate->last_replay_fps = replay_fps;
		}
	}
}

static void stage_give_clear_bonus(const StageInfo *stage, StageClearBonus *bonus) {
	memset(bonus, 0, sizeof(*bonus));

	// FIXME: this is clunky...
	if(!global.is_practice_mode && stage->type == STAGE_STORY) {
		StageInfo *next = stageinfo_get_by_id(stage->id + 1);

		if(next == NULL || next->type != STAGE_STORY) {
			bonus->all_clear = true;
		}
	}

	if(stage->type == STAGE_STORY) {
		bonus->base = stage->id * 1000000;
	}

	if(bonus->all_clear) {
		bonus->base += global.plr.point_item_value * 100;
		// TODO redesign this
		// bonus->graze = global.plr.graze * (global.plr.point_item_value / 10);
	}

	bonus->voltage = imax(0, (int)global.plr.voltage - (int)global.voltage_threshold) * (global.plr.point_item_value / 25);
	bonus->lives = global.plr.lives * global.plr.point_item_value * 5;

	// TODO: maybe a difficulty multiplier?

	bonus->total = bonus->base + bonus->voltage + bonus->lives + bonus->graze;
	player_add_points(&global.plr, bonus->total, global.plr.pos);
}

INLINE bool stage_should_yield(void) {
	return (global.boss && !boss_is_fleeing(global.boss)) || dialog_is_active(global.dialog);
}

static LogicFrameAction stage_logic_frame(void *arg) {
	StageFrameState *fstate = arg;
	StageInfo *stage = fstate->stage;

	++fstate->logic_calls;

	stage_update_fps(fstate);

	if(stage_is_skip_mode()) {
		global.plr.iddqd = true;
	}

	fapproach_p(&fstate->view_shake, 0, 1);
	fapproach_asymptotic_p(&fstate->view_shake, 0, 0.05, 1e-2);

	if(global.replay.input.replay != NULL) {
		replay_input();
	} else {
		stage_input();
	}

	if(global.gameover != GAMEOVER_TRANSITIONING) {
		cosched_run_tasks(&fstate->sched);

		if(!stage_should_yield()) {
			stage->procs->event();
		}

		if(global.gameover == GAMEOVER_SCORESCREEN && global.frames - global.gameover_time == GAMEOVER_SCORE_DELAY) {
			StageClearBonus b;
			stage_give_clear_bonus(stage, &b);
			stage_display_clear_screen(&b);
		}

		if(stage->type == STAGE_SPELL && !global.boss && !fstate->transition_delay) {
			fstate->transition_delay = 120;
		}

		stage->procs->update();
	}

	uint16_t desync_check = (rng_u64() ^ global.plr.points) & 0xFFFF;
	ReplaySyncStatus rpsync = replay_state_check_desync(&global.replay.input, global.frames, desync_check);

	if(
		global.replay.output.stage &&
		fstate->desync_check_freq > 0 &&
		!(global.frames % fstate->desync_check_freq)
	) {
		replay_stage_event(global.replay.output.stage, global.frames, EV_CHECK_DESYNC, desync_check);
	}

	if(
		rpsync == REPLAY_SYNC_FAIL &&
		global.is_replay_verification &&
		!global.replay.output.stage
	) {
		exit(1);
	}

	stage_logic();

	if(fstate->transition_delay) {
		if(!--fstate->transition_delay) {
			stage_finish(GAMEOVER_WIN);
		}
	} else {
		update_transition();
	}

	if(global.replay.input.replay == NULL && global.plr.points > progress.hiscore) {
		progress.hiscore = global.plr.points;
	}

	if(global.gameover > 0) {
		return LFRAME_STOP;
	}

	LogicFrameAction skipmode = skipstate_handle_frame();
	if(skipmode != LFRAME_WAIT) {
		return skipmode;
	}

	if(global.frameskip || (global.replay.input.replay && gamekeypressed(KEY_SKIP))) {
		return LFRAME_SKIP;
	}

	return LFRAME_WAIT;
}

static RenderFrameAction stage_render_frame(void *arg) {
	StageFrameState *fstate = arg;
	StageInfo *stage = fstate->stage;

	if(stage_is_skip_mode()) {
		return RFRAME_DROP;
	}

	rng_lock(&global.rand_game);
	rng_make_active(&global.rand_visual);
	BEGIN_DRAW_CODE();
	stage_draw_scene(stage);
	END_DRAW_CODE();
	rng_unlock(&global.rand_game);
	rng_make_active(&global.rand_game);
	draw_transition();

	return RFRAME_SWAP;
}

static void stage_end_loop(void *ctx);

static void stage_stub_proc(void) { }

void stage_enter(StageInfo *stage, CallChain next) {
	assert(stage);
	assert(stage->procs);

	#define STUB_PROC(proc, stub) do {\
		if(!stage->procs->proc) { \
			stage->procs->proc = stub; \
			log_debug(#proc " proc is missing"); \
		} \
	} while(0)

	static const ShaderRule shader_rules_stub[1] = { NULL };

	STUB_PROC(preload, stage_stub_proc);
	STUB_PROC(begin, stage_stub_proc);
	STUB_PROC(end, stage_stub_proc);
	STUB_PROC(draw, stage_stub_proc);
	STUB_PROC(event, stage_stub_proc);
	STUB_PROC(update, stage_stub_proc);
	STUB_PROC(shader_rules, (ShaderRule*)shader_rules_stub);

	if(global.gameover == GAMEOVER_WIN) {
		global.gameover = 0;
	} else if(global.gameover) {
		run_call_chain(&next, NULL);
		return;
	}

	// I really want to separate all of the game state from the global struct sometime
	global.stage = stage;

	ent_init();
	stage_objpools_alloc();
	stage_draw_pre_init();
	stage_preload();
	stage_draw_init();

	rng_make_active(&global.rand_game);
	stage_start(stage);

	uint64_t start_time, seed;

	if(global.replay.input.replay) {
		ReplayStage *rstg = global.replay.input.stage;

		assert(rstg != NULL);
		assert(stageinfo_get_by_id(rstg->stage) == stage);

		start_time = rstg->start_time;
		seed = rstg->rng_seed;
		global.diff = rstg->diff;

		log_debug("REPLAY_PLAY mode: %d events, stage: \"%s\"", rstg->events.num_elements, stage->title);
	} else {
		start_time = (uint64_t)time(0);
		seed = makeseed();

		StageProgress *p = stageinfo_get_progress(stage, global.diff, true);

		if(p) {
			log_debug("You played this stage %u times", p->num_played);
			log_debug("You cleared this stage %u times", p->num_cleared);

			++p->num_played;
			p->unlocked = true;
		}
	}

	rng_seed(&global.rand_game, seed);

	if(global.replay.input.replay) {
		player_init(&global.plr);
		replay_stage_sync_player_state(global.replay.input.stage, &global.plr);
	}

	if(global.replay.output.replay) {
		global.replay.output.stage = replay_stage_new(
			global.replay.output.replay,
			stage,
			start_time,
			seed,
			global.diff,
			&global.plr
		);
		player_init(&global.plr);
		replay_stage_sync_player_state(global.replay.output.stage, &global.plr);
	}

	StageFrameState *fstate = calloc(1 , sizeof(*fstate));
	cosched_init(&fstate->sched);
	cosched_set_invoke_target(&fstate->sched);
	fstate->stage = stage;
	fstate->cc = next;
	fstate->desync_check_freq = env_get("TAISEI_REPLAY_DESYNC_CHECK_FREQUENCY", FPS * 5);

	_current_stage_state = fstate;

	skipstate_init();

	stage->procs->begin();
	player_stage_post_init(&global.plr);

	if(global.stage->type != STAGE_SPELL) {
		display_stage_title(stage);
	}

	eventloop_enter(fstate, stage_logic_frame, stage_render_frame, stage_end_loop, FPS);
}

void stage_end_loop(void* ctx) {
	StageFrameState *s = ctx;
	assert(s == _current_stage_state);

	if(global.replay.output.replay) {
		replay_stage_event(global.replay.output.stage, global.frames, EV_OVER, 0);
		global.replay.output.stage->plr_points_final = global.plr.points;

		if(global.gameover == GAMEOVER_WIN) {
			global.replay.output.stage->flags |= REPLAY_SFLAG_CLEAR;
		}
	}

	s->stage->procs->end();
	stage_draw_shutdown();
	cosched_finish(&s->sched);
	stage_free();
	player_free(&global.plr);
	free_all_refs();
	ent_shutdown();
	rng_make_active(&global.rand_visual);
	stage_objpools_free();
	stop_all_sfx();

	taisei_commit_persistent_data();
	skipstate_shutdown();

	if(taisei_quit_requested()) {
		global.gameover = GAMEOVER_ABORT;
	}

	_current_stage_state = NULL;

	CallChain cc = s->cc;
	free(s);

	run_call_chain(&cc, NULL);
}

void stage_unlock_bgm(const char *bgm) {
	if(global.replay.input.replay == NULL && !global.plr.stats.total.continues_used) {
		progress_unlock_bgm(bgm);
	}
}

void stage_shake_view(float strength) {
	assume(strength >= 0);
	_current_stage_state->view_shake += strength;
}

float stage_get_view_shake_strength(void) {
	if(_current_stage_state) {
		return _current_stage_state->view_shake;
	}

	return 0;
}
