/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "common.h"
#include "global.h"
#include "menu.h"
#include "savereplay.h"
#include "difficultyselect.h"
#include "charselect.h"
#include "ending.h"
#include "credits.h"
#include "mainmenu.h"
#include "video.h"

typedef struct StartGameContext {
	StageInfo *restart_stage;
	StageInfo *current_stage;
	MenuData *diff_menu;
	MenuData *char_menu;
	Difficulty difficulty;
} StartGameContext;

static void start_game_do_pick_character(CallChainResult ccr);
static void start_game_do_enter_stage(CallChainResult ccr);
static void start_game_do_leave_stage(CallChainResult ccr);
static void start_game_do_show_ending(CallChainResult ccr);
static void start_game_do_show_credits(CallChainResult ccr);
static void start_game_do_cleanup(CallChainResult ccr);

static void start_game_internal(MenuData *menu, StageInfo *info, bool difficulty_menu) {
	StartGameContext *ctx = calloc(1, sizeof(*ctx));

	if(info == NULL) {
		global.is_practice_mode = false;
		ctx->current_stage = stages;
		ctx->restart_stage = stages;
	} else {
		global.is_practice_mode = (info->type != STAGE_EXTRA);
		ctx->current_stage = info;
		ctx->restart_stage = info;
	}

	Difficulty stagediff = info ? info->difficulty : D_Any;

	CallChain cc_pick_character = CALLCHAIN(start_game_do_pick_character, ctx);

	if(stagediff == D_Any) {
		if(difficulty_menu) {
			ctx->diff_menu = create_difficulty_menu();
			enter_menu(ctx->diff_menu, cc_pick_character);
		} else {
			ctx->difficulty = progress.game_settings.difficulty;
			run_call_chain(&cc_pick_character, NULL);
		}
	} else {
		ctx->difficulty = stagediff;
		run_call_chain(&cc_pick_character, NULL);
	}
}

static void start_game_do_pick_character(CallChainResult ccr) {
	StartGameContext *ctx = ccr.ctx;
	MenuData *prev_menu = ccr.result;

	if(prev_menu) {
		if(prev_menu->state == MS_Dead) {
			start_game_do_cleanup(ccr);
			return;
		}

		// came here from the difficulty menu - update our setting
		ctx->difficulty = progress.game_settings.difficulty;
	}

	assert(ctx->char_menu == NULL);
	ctx->char_menu = create_char_menu();
	enter_menu(ctx->char_menu, CALLCHAIN(start_game_do_enter_stage, ctx));
}

static void reset_game(StartGameContext *ctx) {
	ctx->current_stage = ctx->restart_stage;

	global.gameover = GAMEOVER_NONE;
	global.replay_stage = NULL;
	replay_destroy(&global.replay);
	replay_init(&global.replay);
	player_init(&global.plr);
	global.plr.mode = plrmode_find(
		progress.game_settings.character,
		progress.game_settings.shotmode
	);
	global.diff = ctx->difficulty;

	assert(global.plr.mode != NULL);
}

static void kill_aux_menus(StartGameContext *ctx) {
	kill_menu(ctx->char_menu);
	kill_menu(ctx->diff_menu);
	ctx->char_menu = ctx->diff_menu = NULL;
}

static void start_game_do_enter_stage(CallChainResult ccr) {
	StartGameContext *ctx = ccr.ctx;
	MenuData *prev_menu = ccr.result;

	if(prev_menu && prev_menu->state == MS_Dead) {
		assert(prev_menu == ctx->char_menu);
		ctx->char_menu = NULL;
		return;
	}

	kill_aux_menus(ctx);
	reset_game(ctx);
	stage_enter(ctx->current_stage, CALLCHAIN(start_game_do_leave_stage, ctx));
}

static void start_game_do_leave_stage(CallChainResult ccr) {
	StartGameContext *ctx = ccr.ctx;

	if(global.gameover == GAMEOVER_RESTART) {
		reset_game(ctx);
		stage_enter(ctx->current_stage, CALLCHAIN(start_game_do_leave_stage, ctx));
		return;
	}

	if(ctx->current_stage->type == STAGE_STORY && !global.is_practice_mode) {
		++ctx->current_stage;

		if(ctx->current_stage->type == STAGE_STORY) {
			stage_enter(ctx->current_stage, CALLCHAIN(start_game_do_leave_stage, ctx));
		} else {
			CallChain cc;

			if(global.gameover == GAMEOVER_WIN) {
				ending_preload();
				credits_preload();
				cc = CALLCHAIN(start_game_do_show_ending, ctx);
			} else {
				cc = CALLCHAIN(start_game_do_cleanup, ctx);
			}

			ask_save_replay(cc);
		}
	} else {
		ask_save_replay(CALLCHAIN(start_game_do_cleanup, ctx));
	}
}

static void start_game_do_show_ending(CallChainResult ccr) {
	ending_enter(CALLCHAIN(start_game_do_show_credits, ccr.ctx));
}

static void start_game_do_show_credits(CallChainResult ccr) {
	credits_enter(CALLCHAIN(start_game_do_cleanup, ccr.ctx));
}

static void start_game_do_cleanup(CallChainResult ccr) {
	StartGameContext *ctx = ccr.ctx;
	kill_aux_menus(ctx);
	free(ctx);
	free_resources(false);
	global.replay_stage = NULL;
	global.gameover = GAMEOVER_NONE;
	replay_destroy(&global.replay);
	main_menu_update_practice_menus();
	start_bgm("menu");
}

void start_game(MenuData *m, void *arg) {
	start_game_internal(m, (StageInfo*)arg, true);
}

void start_game_no_difficulty_menu(MenuData *m, void *arg) {
	start_game_internal(m, (StageInfo*)arg, false);
}

void draw_menu_selector(float x, float y, float w, float h, float t) {
	Sprite *bg = get_sprite("part/smoke");
	r_mat_push();
	r_mat_translate(x, y, 0);
	r_mat_scale(w / bg->w, h / bg->h, 1);
	r_mat_rotate_deg(t*2,0,0,1);
	r_color4(0,0,0,0.5 * (1 - transition.fade));
	draw_sprite(0, 0, "part/smoke");
	r_mat_pop();
}

void draw_menu_title(MenuData *m, const char *title) {
	text_draw(title, &(TextParams) {
		.pos = { (text_width(get_font("big"), title, 0) + 10) * (1.0 - menu_fade(m)), 30 },
		.align = ALIGN_RIGHT,
		.font = "big",
		.color = RGB(1, 1, 1),
		.shader = "text_default",
	});
}

void draw_menu_list(MenuData *m, float x, float y, void (*draw)(void*, int, int)) {
	r_mat_push();
	float offset = ((((m->ecount+5) * 20) > SCREEN_H)? min(0, SCREEN_H * 0.7 - y - m->drawdata[2]) : 0);
	r_mat_translate(x, y + offset, 0);

	draw_menu_selector(m->drawdata[0], m->drawdata[2], m->drawdata[1], 34, m->frames);

	for(int i = 0; i < m->ecount; ++i) {
		MenuEntry *e = &(m->entries[i]);

		float p = offset + 20*i;

		if(p < -y-10 || p > SCREEN_H+10)
			continue;

		float a = e->drawdata * 0.1;
		float o = (p < 0? 1-p/(-y-10) : 1);

		Color clr;

		if(e->action == NULL) {
			clr = *RGBA_MUL_ALPHA(0.5, 0.5, 0.5, 0.5*o);
		} else {
			float ia = 1-a;
			clr = *RGBA_MUL_ALPHA(0.9 + ia * 0.1, 0.6 + ia * 0.4, 0.2 + ia * 0.8, (0.7 + 0.3 * a)*o);
		}

		r_color(&clr);

		if(draw && i < m->ecount-1) {
			draw(e, i, m->ecount);
		} else if(e->name) {
			ShaderProgram *sh_prev = r_shader_current();
			r_shader("text_default");
			text_draw(e->name, &(TextParams) { .pos = { 20 - e->drawdata, 20*i } });
			r_shader_ptr(sh_prev);
		}
	}

	r_mat_pop();
}

void animate_menu_list_entry(MenuData *m, int i) {
	MenuEntry *e = &(m->entries[i]);
	e->drawdata += 0.2 * (10*(i == m->cursor) - e->drawdata);
}

void animate_menu_list_entries(MenuData *m) {
	for(int i = 0; i < m->ecount; ++i) {
		animate_menu_list_entry(m, i);
	}
}

void animate_menu_list(MenuData *m) {
	MenuEntry *s = m->entries + m->cursor;
	int w = text_width(get_font("standard"), s->name, 0);

	m->drawdata[0] += (10 + w/2.0 - m->drawdata[0])/10.0;
	m->drawdata[1] += (w*2 - m->drawdata[1])/10.0;
	m->drawdata[2] += (20*m->cursor - m->drawdata[2])/10.0;

	animate_menu_list_entries(m);
}

void menu_action_close(MenuData *menu, void *arg) {
	menu->state = MS_Dead;
}
