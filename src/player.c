/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "player.h"

#include "projectile.h"
#include "global.h"
#include "plrmodes.h"
#include "stage.h"
#include "stagetext.h"
#include "stagedraw.h"
#include "entity.h"

void player_init(Player *plr) {
	memset(plr, 0, sizeof(Player));
	plr->pos = VIEWPORT_W/2 + I*(VIEWPORT_H-64);
	plr->lives = PLR_START_LIVES;
	plr->bombs = PLR_START_BOMBS;
	plr->point_item_value = PLR_START_PIV;
	plr->power = 100;
	plr->deathtime = -1;
	plr->continuetime = -1;
	plr->mode = plrmode_find(0, 0);
}

void player_stage_pre_init(Player *plr) {
	plr->recovery = 0;
	plr->respawntime = 0;
	plr->deathtime = -1;
	plr->axis_lr = 0;
	plr->axis_ud = 0;
	plrmode_preload(plr->mode);
}

double player_property(Player *plr, PlrProperty prop) {
	return plr->mode->procs.property(plr, prop);
}

static void ent_draw_player(EntityInterface *ent);
static DamageResult ent_damage_player(EntityInterface *ent, const DamageInfo *dmg);

static void player_spawn_focus_circle(Player *plr);

void player_stage_post_init(Player *plr) {
	assert(plr->mode != NULL);

	// ensure the essential callbacks are there. other code tests only for the optional ones
	assert(plr->mode->procs.shot != NULL);
	assert(plr->mode->procs.bomb != NULL);
	assert(plr->mode->procs.property != NULL);
	assert(plr->mode->character != NULL);
	assert(plr->mode->dialog != NULL);

	delete_enemies(&global.plr.slaves);

	if(plr->mode->procs.init != NULL) {
		plr->mode->procs.init(plr);
	}

	aniplayer_create(&plr->ani, get_ani(plr->mode->character->player_sprite_name), "main");

	plr->ent.draw_layer = LAYER_PLAYER;
	plr->ent.draw_func = ent_draw_player;
	plr->ent.damage_func = ent_damage_player;
	ent_register(&plr->ent, ENT_PLAYER);

	player_spawn_focus_circle(plr);

	plr->extralife_threshold = player_next_extralife_threshold(plr->extralives_given);

	while(plr->points >= plr->extralife_threshold) {
		plr->extralife_threshold = player_next_extralife_threshold(++plr->extralives_given);
	}
}

void player_free(Player *plr) {
	if(plr->mode->procs.free) {
		plr->mode->procs.free(plr);
	}

	aniplayer_free(&plr->ani);
	ent_unregister(&plr->ent);
	delete_enemy(&plr->focus_circle, plr->focus_circle.first);
}

static void player_full_power(Player *plr) {
	play_sound("full_power");
	stage_clear_hazards(CLEAR_HAZARDS_ALL);
	stagetext_add("Full Power!", VIEWPORT_W * 0.5 + VIEWPORT_H * 0.33 * I, ALIGN_CENTER, get_font("big"), RGB(1, 1, 1), 0, 60, 20, 20);
}

bool player_set_power(Player *plr, short npow) {
	int pow_base = clamp(npow, 0, PLR_MAX_POWER);
	int pow_overflow = clamp(npow - PLR_MAX_POWER, 0, PLR_MAX_POWER_OVERFLOW);

	if(plr->mode->procs.power) {
		plr->mode->procs.power(plr, pow_base);
	}

	int oldpow = plr->power;
	int oldpow_over = plr->power_overflow;

	plr->power = pow_base;
	plr->power_overflow = pow_overflow;

	if((oldpow + oldpow_over) / 100 < (pow_base + pow_overflow) / 100) {
		play_sound("powerup");
	}

	if(plr->power == PLR_MAX_POWER && oldpow < PLR_MAX_POWER) {
		player_full_power(plr);
	}

	return (oldpow + oldpow_over) != (plr->power + plr->power_overflow);
}

bool player_add_power(Player *plr, short pdelta) {
	return player_set_power(plr, plr->power + plr->power_overflow + pdelta);
}

void player_move(Player *plr, complex delta) {
	delta *= player_property(plr, PLR_PROP_SPEED);
	complex lastpos = plr->pos;
	double x = clamp(creal(plr->pos) + creal(delta), PLR_MIN_BORDER_DIST, VIEWPORT_W - PLR_MIN_BORDER_DIST);
	double y = clamp(cimag(plr->pos) + cimag(delta), PLR_MIN_BORDER_DIST, VIEWPORT_H - PLR_MIN_BORDER_DIST);
	plr->pos = x + y*I;
	complex realdir = plr->pos - lastpos;

	if(cabs(realdir)) {
		plr->lastmovedir = realdir / cabs(realdir);
	}

	plr->velocity = realdir;
}

static void ent_draw_player(EntityInterface *ent) {
	Player *plr = ENT_CAST(ent, Player);

	if(plr->deathtime > global.frames) {
		return;
	}

	if(plr->focus) {
		r_draw_sprite(&(SpriteParams) {
			.sprite = "fairy_circle",
			.rotation.angle = DEG2RAD * global.frames * 10,
			.color = RGBA_MUL_ALPHA(1, 1, 1, 0.2 * (clamp(plr->focus, 0, 15) / 15.0)),
			.pos = { creal(plr->pos), cimag(plr->pos) },
		});
	}

	Color c;

	if(!player_is_vulnerable(plr)) {
		float f = 0.3*sin(0.1*global.frames);
		c = *RGBA_MUL_ALPHA(1.0+f, 1.0, 1.0-f, 0.7+f);
	} else {
		c = *RGBA_MUL_ALPHA(1.0, 1.0, 1.0, 1.0);
	}

	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = aniplayer_get_frame(&plr->ani),
		.pos = { creal(plr->pos), cimag(plr->pos) },
		.color = &c,
	});
}

static int player_focus_circle_logic(Enemy *e, int t) {
	if(t < 0) {
		return ACTION_NONE;
	}

	double alpha = creal(e->args[0]);

	if(t <= 1) {
		alpha = min(0.1, alpha);
	} else {
		alpha = approach(alpha, (global.plr.inputflags & INFLAG_FOCUS) ? 1 : 0, 1/30.0);
	}

	e->args[0] = alpha;
	return ACTION_NONE;
}

static void player_focus_circle_visual(Enemy *e, int t, bool render) {
	if(!render) {
		return;
	}

	float focus_opacity = creal(e->args[0]);

	if(focus_opacity > 0) {
		int trans_frames = 12;
		double trans_factor = 1 - min(trans_frames, t) / (double)trans_frames;
		double rot_speed = DEG2RAD * global.frames * (1 + 3 * trans_factor);
		double scale = 1.0 + trans_factor;

		r_draw_sprite(&(SpriteParams) {
			.sprite = "focus",
			.rotation.angle = rot_speed,
			.color = RGBA_MUL_ALPHA(1, 1, 1, creal(e->args[0])),
			.pos = { creal(e->pos), cimag(e->pos) },
			.scale.both = scale,
		});

		r_draw_sprite(&(SpriteParams) {
			.sprite = "focus",
			.rotation.angle = rot_speed * -1,
			.color = RGBA_MUL_ALPHA(1, 1, 1, creal(e->args[0])),
			.pos = { creal(e->pos), cimag(e->pos) },
			.scale.both = scale,
		});
	}

	float ps_opacity = 1.0;
	float ps_fill_factor = 1.0;

	if(player_is_powersurge_active(&global.plr)) {
		ps_opacity *= clamp((global.frames - global.plr.powersurge.time.activated) / 30.0, 0, 1);
	} else if(global.plr.powersurge.time.expired == 0) {
		ps_opacity = 0;
	} else {
		ps_opacity *= (1 - clamp((global.frames - global.plr.powersurge.time.expired) / 40.0, 0, 1));
		ps_fill_factor = pow(ps_opacity, 2);
	}

	if(ps_opacity > 0) {
		r_state_push();
		r_mat_push();
		r_mat_translate(creal(e->pos), cimag(e->pos), 0);
		r_mat_scale(140, 140, 0);
		r_shader("healthbar_radial");
		r_uniform_vec4_rgba("borderColor",   RGBA(0.5, 0.5, 0.5, 0.5));
		r_uniform_vec4_rgba("glowColor",     RGBA(0.5, 0.5, 0.5, 0.75));
		r_uniform_vec4_rgba("fillColor",     RGBA(1.5, 0.5, 0.0, 0.75));
		r_uniform_vec4_rgba("altFillColor",  RGBA(0.0, 0.5, 1.5, 0.75));
		r_uniform_vec4_rgba("coreFillColor", RGBA(0.8, 0.8, 0.8, 0.25));
		r_uniform_vec2("fill", global.plr.powersurge.positive * ps_fill_factor, global.plr.powersurge.negative * ps_fill_factor);
		r_uniform_float("opacity", ps_opacity);
		r_draw_quad();
		r_mat_pop();
		r_state_pop();

		char buf[64];
		format_huge_num(0, global.plr.powersurge.bonus.baseline, sizeof(buf), buf);
		Font *fnt = get_font("monotiny");

		float x = creal(e->pos);
		float y = cimag(e->pos) + 80;

		float text_opacity = ps_opacity * 0.75;

		x += text_draw(buf, &(TextParams) {
			.shader = "text_hud",
			.font_ptr = fnt,
			.pos = { x, y },
			.color = RGBA_MUL_ALPHA(1.0, 1.0, 1.0, text_opacity),
			.align = ALIGN_CENTER,
		});

		snprintf(buf, sizeof(buf), " +%u", global.plr.powersurge.bonus.gain_rate);

		text_draw(buf, &(TextParams) {
			.shader = "text_hud",
			.font_ptr = fnt,
			.pos = { x, y },
			.color = RGBA_MUL_ALPHA(0.3, 0.6, 1.0, text_opacity),
			.align = ALIGN_LEFT,
		});

		r_shader("sprite_filled_circle");
		r_uniform_vec4("color_inner", 0, 0, 0, 0);
		r_uniform_vec4("color_outer", 0, 0, 0.1 * ps_opacity, 0.1 * ps_opacity);
	}
}

static void player_spawn_focus_circle(Player *plr) {
	Enemy *f = create_enemy_p(&plr->focus_circle, 0, ENEMY_IMMUNE, player_focus_circle_visual, player_focus_circle_logic, 0, 0, 0, 0);
	f->ent.draw_layer = LAYER_PLAYER_FOCUS;
}

static void player_fail_spell(Player *plr) {
	if( !global.boss ||
		!global.boss->current ||
		global.boss->current->finished ||
		global.boss->current->failtime ||
		global.boss->current->starttime >= global.frames ||
		global.stage->type == STAGE_SPELL
	) {
		return;
	}

	global.boss->current->failtime = global.frames;

	if(global.boss->current->type == AT_ExtraSpell) {
		boss_finish_current_attack(global.boss);
	}
}

bool player_should_shoot(Player *plr, bool extra) {
	return
		(plr->inputflags & INFLAG_SHOT) &&
		!global.dialog
		&& player_is_alive(&global.plr)
		/* && (!extra || !player_is_bomb_active(plr)) */;
}

void player_placeholder_bomb_logic(Player *plr) {
	if(!player_is_bomb_active(plr)) {
		return;
	}

	DamageInfo dmg;
	dmg.amount = 100;
	dmg.type = DMG_PLAYER_BOMB;

	for(Enemy *en = global.enemies.first; en; en = en->next) {
		ent_damage(&en->ent, &dmg);
	}

	if(global.boss) {
		ent_damage(&global.boss->ent, &dmg);
	}

	stage_clear_hazards(CLEAR_HAZARDS_ALL);
}

static void player_powersurge_expired(Player *plr);

static void _powersurge_trail_draw(Projectile *p, float t, float cmul) {
	float nt = t / p->timeout;
	float s = 1 + (2 + 0.5 * psin((t+global.frames*1.23)/5.0)) * nt * nt;

	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = p->sprite,
		.scale.both = s,
		.pos = { creal(p->pos), cimag(p->pos) },
		.color = color_mul_scalar(RGBA(0.8, 0.1 + 0.2 * psin((t+global.frames)/5.0), 0.1, 0.0), 0.5 * (1 - nt) * cmul),
		.shader_params = &(ShaderCustomParams){{ -2 * nt * nt }},
		.shader = "sprite_silhouette",
	});
}

static void powersurge_trail_draw(Projectile *p, int t) {
	if(t > 0) {
		_powersurge_trail_draw(p, t - 0.5, 0.25);
		_powersurge_trail_draw(p, t, 0.25);
	} else {
		_powersurge_trail_draw(p, t, 0.5);
	}
}

static int powersurge_trail(Projectile *p, int t) {
	if(t == EVENT_BIRTH) {
		return ACTION_ACK;
	}

	if(t == EVENT_DEATH) {
		free(p->sprite);
		return ACTION_ACK;
	}

	complex v = (global.plr.pos - p->pos) * 0.05;
	p->args[0] += (v - p->args[0]) * (1 - t / p->timeout);
	p->pos += p->args[0];

	return ACTION_NONE;
}

void player_powersurge_calc_bonus(Player *plr, PowerSurgeBonus *b) {
	b->gain_rate = round(1000 * plr->powersurge.negative * plr->powersurge.negative);
	b->baseline = plr->powersurge.power + plr->powersurge.damage_done * 0.8;
	b->score = b->baseline;
	b->discharge_power = sqrtf(0.2 * b->baseline + 1024 * log1pf(b->baseline)) * smoothstep(0, 1, 0.0001 * b->baseline);
	b->discharge_range = 1.2 * b->discharge_power;
	b->discharge_damage = 10 * pow(b->discharge_power, 1.1);
}

static int powersurge_charge_particle(Projectile *p, int t) {
	if(t == EVENT_BIRTH) {
		return ACTION_ACK;
	}

	if(t == EVENT_DEATH) {
		return ACTION_ACK;
	}

	Player *plr = &global.plr;

	if(player_is_alive(plr)) {
		p->pos = plr->pos;
	}

	return ACTION_NONE;
}

static void player_powersurge_logic(Player *plr) {
	plr->powersurge.positive = max(0, plr->powersurge.positive - lerp(PLR_POWERSURGE_POSITIVE_DRAIN_MIN, PLR_POWERSURGE_POSITIVE_DRAIN_MAX, plr->powersurge.positive));
	plr->powersurge.negative = max(0, plr->powersurge.negative - lerp(PLR_POWERSURGE_NEGATIVE_DRAIN_MIN, PLR_POWERSURGE_NEGATIVE_DRAIN_MAX, plr->powersurge.negative));

	if(stage_is_cleared()) {
		player_cancel_powersurge(plr);
	}

	if(plr->powersurge.positive <= plr->powersurge.negative) {
		player_powersurge_expired(plr);
		return;
	}

	player_powersurge_calc_bonus(plr, &plr->powersurge.bonus);

	PARTICLE(
		.sprite_ptr = memdup(aniplayer_get_frame(&plr->ani), sizeof(Sprite)),
		.pos = plr->pos,
		.color = RGBA(1, 1, 1, 0.5),
		.rule = powersurge_trail,
		.draw_rule = powersurge_trail_draw,
		.timeout = 15,
		.layer = LAYER_PARTICLE_HIGH,
		.flags = PFLAG_NOREFLECT,
	);

	if(!(global.frames % 6)) {
		Sprite *spr = get_sprite("part/powersurge_field");
		double scale = 2 * plr->powersurge.bonus.discharge_range / spr->w;
		double angle = frand() * 2 * M_PI;

		PARTICLE(
			.sprite_ptr = spr,
			.pos = plr->pos,
			.color = color_mul_scalar(frand() < 0.5 ? RGBA(1.5, 0.5, 0.0, 0.1) : RGBA(0.0, 0.5, 1.5, 0.1), 0.25),
			.rule = powersurge_charge_particle,
			.draw_rule = ScaleFade,
			.timeout = 14,
			.angle = angle,
			.args = { 0, 0, (1+I)*scale, 0},
			.layer = LAYER_PLAYER - 1,
			.flags = PFLAG_NOREFLECT,
		);

		PARTICLE(
			.sprite_ptr = spr,
			.pos = plr->pos,
			.color = RGBA(0.5, 0.5, 0.5, 0),
			.rule = powersurge_charge_particle,
			.draw_rule = ScaleFade,
			.timeout = 3,
			.angle = angle,
			.args = { 0, 0, (1+I)*scale, 0},
			.layer = LAYER_PLAYER - 1,
			.flags = PFLAG_NOREFLECT,
		);
	}

	plr->powersurge.power += plr->powersurge.bonus.gain_rate;
}

void player_logic(Player* plr) {
	if(plr->continuetime == global.frames) {
		plr->lives = PLR_START_LIVES;
		plr->bombs = PLR_START_BOMBS;
		plr->point_item_value = PLR_START_PIV;
		plr->life_fragments = 0;
		plr->bomb_fragments = 0;
		plr->continues_used += 1;
		player_set_power(plr, 0);
		stage_clear_hazards(CLEAR_HAZARDS_ALL);
		spawn_items(plr->deathpos, ITEM_POWER, (int)ceil(PLR_MAX_POWER/(double)POWER_VALUE), NULL);
	}

	process_enemies(&plr->slaves);
	aniplayer_update(&plr->ani);

	if(plr->deathtime < -1) {
		plr->deathtime++;
		plr->pos -= I;
		stage_clear_hazards(CLEAR_HAZARDS_ALL | CLEAR_HAZARDS_NOW);
		return;
	}

	if(player_is_powersurge_active(plr)) {
		player_powersurge_logic(plr);
	}

	plr->focus = approach(plr->focus, (plr->inputflags & INFLAG_FOCUS) ? 30 : 0, 1);
	plr->focus_circle.first->pos = plr->pos;
	process_enemies(&plr->focus_circle);

	if(plr->mode->procs.think) {
		plr->mode->procs.think(plr);
	}

	if(player_should_shoot(plr, false)) {
		plr->mode->procs.shot(plr);
	}

	if(global.frames == plr->deathtime) {
		player_realdeath(plr);
	} else if(plr->deathtime > global.frames) {
		stage_clear_hazards(CLEAR_HAZARDS_ALL | CLEAR_HAZARDS_NOW);
	}

	if(player_is_bomb_active(plr)) {
		if(plr->bombcanceltime) {
			int bctime = plr->bombcanceltime + plr->bombcanceldelay;

			if(bctime <= global.frames) {
				plr->recovery = global.frames;
				plr->bombcanceltime = 0;
				plr->bombcanceldelay = 0;
				return;
			}
		}

		player_fail_spell(plr);
	}
}

static bool player_bomb(Player *plr) {
	if(global.boss && global.boss->current && global.boss->current->type == AT_ExtraSpell)
		return false;

	int bomb_time = floor(player_property(plr, PLR_PROP_BOMB_TIME));

	if(bomb_time <= 0) {
		return false;
	}

	if(!player_is_bomb_active(plr) && (plr->bombs > 0 || plr->iddqd) && global.frames - plr->respawntime >= 60) {
		player_fail_spell(plr);
		// player_cancel_powersurge(plr);
		stage_clear_hazards(CLEAR_HAZARDS_ALL);

		plr->mode->procs.bomb(plr);
		plr->bombs--;

		if(plr->deathtime > 0) {
			plr->deathtime = -1;

			if(plr->bombs) {
				plr->bombs--;
			}
		}

		if(plr->bombs < 0) {
			plr->bombs = 0;
		}

		plr->bombtotaltime = bomb_time;
		plr->recovery = global.frames + plr->bombtotaltime;
		plr->bombcanceltime = 0;
		plr->bombcanceldelay = 0;

		collect_all_items(1);
		return true;
	}

	return false;
}

static bool player_powersurge(Player *plr) {
	if(
		!player_is_alive(plr) ||
		player_is_bomb_active(plr) ||
		player_is_powersurge_active(plr) ||
		plr->power + plr->power_overflow < PLR_POWERSURGE_POWERCOST
	) {
		return false;
	}

	plr->powersurge.positive = 1.0;
	plr->powersurge.negative = 0.0;
	plr->powersurge.time.activated = global.frames;
	plr->powersurge.power = 0;
	plr->powersurge.damage_accum = 0;
	player_add_power(plr, -PLR_POWERSURGE_POWERCOST);

	collect_all_items(1);
	stagetext_add("Power Surge!", plr->pos - 64 * I, ALIGN_CENTER, get_font("standard"), RGBA(0.75, 0.75, 0.75, 0.75), 0, 45, 10, 20);

	return true;
}

bool player_is_powersurge_active(Player *plr) {
	return plr->powersurge.positive > plr->powersurge.negative;
}

bool player_is_bomb_active(Player *plr) {
	return global.frames - plr->recovery < 0;
}

bool player_is_vulnerable(Player *plr) {
	return global.frames - abs(plr->recovery) >= 0 && !plr->iddqd && player_is_alive(plr);
}

bool player_is_alive(Player *plr) {
	return plr->deathtime >= -1 && plr->deathtime < global.frames;
}

void player_cancel_bomb(Player *plr, int delay) {
	if(!player_is_bomb_active(plr)) {
		return;
	}

	if(plr->bombcanceltime) {
		int canceltime_queued = plr->bombcanceltime + plr->bombcanceldelay;
		int canceltime_requested = global.frames + delay;

		if(canceltime_queued > canceltime_requested) {
			plr->bombcanceldelay -= (canceltime_queued - canceltime_requested);
		}
	} else {
		plr->bombcanceltime = global.frames;
		plr->bombcanceldelay = delay;
	}
}

static void player_powersurge_expired(Player *plr) {
	plr->powersurge.time.expired = global.frames;

	PowerSurgeBonus bonus;
	player_powersurge_calc_bonus(plr, &bonus);

	Sprite *blast = get_sprite("part/blast_huge_halo");
	float scale = 2 * bonus.discharge_range / blast->w;

	PARTICLE(
		.sprite_ptr = blast,
		.pos = plr->pos,
		.color = RGBA(0.6, 1.0, 4.4, 0.0),
		.draw_rule = ScaleFade,
		.timeout = 20,
		.args = { 0, 0, scale * (2 + 0 * I) },
		.angle = M_PI*2*frand(),
	);

	player_add_points(&global.plr, bonus.score);
	ent_area_damage(plr->pos, bonus.discharge_range, &(DamageInfo) { bonus.discharge_damage, DMG_PLAYER_DISCHARGE }, NULL, NULL);
	stage_clear_hazards_at(plr->pos, bonus.discharge_range, CLEAR_HAZARDS_ALL | CLEAR_HAZARDS_NOW | CLEAR_HAZARDS_SPAWN_VOLTAGE);

	log_debug(
		"Power Surge expired at %i (duration: %i); baseline = %u; score = %u; discharge = %g, dmg = %g, range = %g",
		plr->powersurge.time.expired,
		plr->powersurge.time.expired - plr->powersurge.time.activated,
		bonus.baseline,
		bonus.score,
		bonus.discharge_power,
		bonus.discharge_damage,
		bonus.discharge_range
	);

	plr->powersurge.damage_done = 0;
}

void player_cancel_powersurge(Player *plr) {
	if(player_is_powersurge_active(plr)) {
		plr->powersurge.positive = plr->powersurge.negative;
		player_powersurge_expired(plr);
	}
}

void player_extend_powersurge(Player *plr, float pos, float neg) {
	if(player_is_powersurge_active(plr)) {
		plr->powersurge.positive = clamp(plr->powersurge.positive + pos, 0, 1);
		plr->powersurge.negative = clamp(plr->powersurge.negative + neg, 0, 1);

		if(plr->powersurge.positive <= plr->powersurge.negative) {
			player_powersurge_expired(plr);
		}
	}
}

double player_get_bomb_progress(Player *plr, double *out_speed) {
	if(!player_is_bomb_active(plr)) {
		if(out_speed != NULL) {
			*out_speed = 1.0;
		}

		return 1;
	}

	int start_time = plr->recovery - plr->bombtotaltime;
	int end_time = plr->recovery;

	if(!plr->bombcanceltime || plr->bombcanceltime + plr->bombcanceldelay >= end_time) {
		if(out_speed != NULL) {
			*out_speed = 1.0;
		}

		return (plr->bombtotaltime - (end_time - global.frames))/(double)plr->bombtotaltime;
	}

	int cancel_time = plr->bombcanceltime + plr->bombcanceldelay;
	int passed_time = plr->bombcanceltime - start_time;

	int shortened_total_time = (plr->bombtotaltime - passed_time) - (end_time - cancel_time);
	int shortened_passed_time = (global.frames - plr->bombcanceltime);

	double passed_fraction = passed_time / (double)plr->bombtotaltime;
	double shortened_fraction = shortened_passed_time / (double)shortened_total_time;
	shortened_fraction *= (1 - passed_fraction);

	if(out_speed != NULL) {
		*out_speed = (plr->bombtotaltime - passed_time) / (double)shortened_total_time;
	}

	return passed_fraction + shortened_fraction;
}

void player_realdeath(Player *plr) {
	plr->deathtime = -DEATH_DELAY-1;
	plr->respawntime = global.frames;
	plr->inputflags &= ~INFLAGS_MOVE;
	plr->deathpos = plr->pos;
	plr->pos = VIEWPORT_W/2 + VIEWPORT_H*I+30.0*I;
	plr->recovery = -(global.frames + DEATH_DELAY + 150);
	stage_clear_hazards(CLEAR_HAZARDS_ALL);

	player_fail_spell(plr);

	if(global.stage->type != STAGE_SPELL && global.boss && global.boss->current && global.boss->current->type == AT_ExtraSpell) {
		// deaths in extra spells "don't count"
		return;
	}

	int total_power = plr->power + plr->power_overflow;

	int drop = max(2, (total_power * 0.15) / POWER_VALUE);
	spawn_items(plr->deathpos, ITEM_POWER, drop, NULL);

	player_set_power(plr, total_power * 0.7);
	plr->bombs = PLR_START_BOMBS;
	plr->bomb_fragments = 0;
	plr->voltage *= 0.9;

	if(plr->lives-- == 0 && global.replaymode != REPLAY_PLAY) {
		stage_gameover();
	}
}

static void player_death_effect_draw_overlay(Projectile *p, int t) {
	FBPair *framebuffers = stage_get_fbpair(FBPAIR_FG);
	r_framebuffer(framebuffers->front);
	r_uniform_sampler("noise_tex", "static");
	r_uniform_int("frames", global.frames);
	r_uniform_float("progress", t / p->timeout);
	r_uniform_vec2("origin", creal(p->pos), VIEWPORT_H - cimag(p->pos));
	r_uniform_vec2("clear_origin", creal(global.plr.pos), VIEWPORT_H - cimag(global.plr.pos));
	r_uniform_vec2("viewport", VIEWPORT_W, VIEWPORT_H);
	draw_framebuffer_tex(framebuffers->back, VIEWPORT_W, VIEWPORT_H);
	fbpair_swap(framebuffers);

	// This change must propagate, hence the r_state salsa. Yes, pop then push, I know what I'm doing.
	r_state_pop();
	r_framebuffer(framebuffers->back);
	r_state_push();
}

static void player_death_effect_draw_sprite(Projectile *p, int t) {
	float s = t / p->timeout;

	float stretch_range = 3, sx, sy;

	sx = 0.5 + 0.5 * cos(M_PI * (2 * pow(s, 0.5) + 1));
	sx = (1 - s) * (1 + (stretch_range - 1) * sx) + s * stretch_range * sx;
	sy = 1 + pow(s, 3);

	if(sx <= 0 || sy <= 0) {
		return;
	}

	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = p->sprite,
		.pos = { creal(p->pos), cimag(p->pos) },
		.scale = { .x = sx, .y = sy },
	});
}

static int player_death_effect(Projectile *p, int t) {
	if(t < 0) {
		if(t == EVENT_DEATH) {
			for(int i = 0; i < 12; ++i) {
				PARTICLE(
					.sprite = "blast",
					.pos = p->pos + 2 * frand() * cexp(I*M_PI*2*frand()),
					.color = RGBA(0.15, 0.2, 0.5, 0),
					.timeout = 12 + i + 2 * nfrand(),
					.draw_rule = GrowFade,
					.args = { 0, 20 + i },
					.angle = M_PI*2*frand(),
					.flags = PFLAG_NOREFLECT,
					.layer = LAYER_OVERLAY,
				);
			}
		}

		return ACTION_ACK;
	}

	return ACTION_NONE;
}

void player_death(Player *plr) {
	if(!player_is_vulnerable(plr) || stage_is_cleared()) {
		return;
	}

	play_sound("death");

	for(int i = 0; i < 60; i++) {
		tsrand_fill(2);
		PARTICLE(
			.sprite = "flare",
			.pos = plr->pos,
			.rule = linear,
			.timeout = 40,
			.draw_rule = Shrink,
			.args = { (3+afrand(0)*7)*cexp(I*tsrand_a(1)) },
			.flags = PFLAG_NOREFLECT,
		);
	}

	stage_clear_hazards(CLEAR_HAZARDS_ALL);

	PARTICLE(
		.sprite = "blast",
		.pos = plr->pos,
		.color = RGBA(0.5, 0.15, 0.15, 0),
		.timeout = 35,
		.draw_rule = GrowFade,
		.args = { 0, 2.4 },
		.flags = PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE,
	);

	PARTICLE(
		.pos = plr->pos,
		.size = 1+I,
		.timeout = 90,
		.draw_rule = player_death_effect_draw_overlay,
		.blend = BLEND_NONE,
		.flags = PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE,
		.layer = LAYER_OVERLAY,
		.shader = "player_death",
	);

	PARTICLE(
		.sprite_ptr = aniplayer_get_frame(&plr->ani),
		.pos = plr->pos,
		.timeout = 30,
		.rule = player_death_effect,
		.draw_rule = player_death_effect_draw_sprite,
		.flags = PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE,
		.layer = LAYER_PLAYER_FOCUS, // LAYER_OVERLAY | 1,
	);

	plr->deathtime = global.frames + floor(player_property(plr, PLR_PROP_DEATHBOMB_WINDOW));

	if(player_is_powersurge_active(plr)) {
		player_cancel_powersurge(plr);
		// player_bomb(plr);
	}
}

static DamageResult ent_damage_player(EntityInterface *ent, const DamageInfo *dmg) {
	Player *plr = ENT_CAST(ent, Player);

	if(
		!player_is_vulnerable(plr) ||
		(dmg->type != DMG_ENEMY_SHOT && dmg->type != DMG_ENEMY_COLLISION)
	) {
		return DMG_RESULT_IMMUNE;
	}

	player_death(plr);
	return DMG_RESULT_OK;
}

void player_damage_hook(Player *plr, EntityInterface *target, DamageInfo *dmg) {
	if(player_is_powersurge_active(plr) && dmg->type == DMG_PLAYER_SHOT) {
		dmg->amount *= 1.2;
	}
}

static PlrInputFlag key_to_inflag(KeyIndex key) {
	switch(key) {
		case KEY_UP:    return INFLAG_UP;
		case KEY_DOWN:  return INFLAG_DOWN;
		case KEY_LEFT:  return INFLAG_LEFT;
		case KEY_RIGHT: return INFLAG_RIGHT;
		case KEY_FOCUS: return INFLAG_FOCUS;
		case KEY_SHOT:  return INFLAG_SHOT;
		case KEY_SKIP:  return INFLAG_SKIP;
		default:        return 0;
	}
}

static bool player_updateinputflags(Player *plr, PlrInputFlag flags) {
	if(flags == plr->inputflags) {
		return false;
	}

	if((flags & INFLAG_FOCUS) && !(plr->inputflags & INFLAG_FOCUS)) {
		plr->focus_circle.first->birthtime = global.frames;
	}

	plr->inputflags = flags;
	return true;
}

static bool player_updateinputflags_moveonly(Player *plr, PlrInputFlag flags) {
	return player_updateinputflags(plr, (flags & INFLAGS_MOVE) | (plr->inputflags & ~INFLAGS_MOVE));
}

static bool player_setinputflag(Player *plr, KeyIndex key, bool mode) {
	PlrInputFlag newflags = plr->inputflags;
	PlrInputFlag keyflag = key_to_inflag(key);

	if(!keyflag) {
		return false;
	}

	if(mode) {
		newflags |= keyflag;
	} else {
		newflags &= ~keyflag;
	}

	return player_updateinputflags(plr, newflags);
}

static bool player_set_axis(int *aptr, uint16_t value) {
	int16_t new = (int16_t)value;

	if(*aptr == new) {
		return false;
	}

	*aptr = new;
	return true;
}

void player_event(Player *plr, uint8_t type, uint16_t value, bool *out_useful, bool *out_cheat) {
	bool useful = true;
	bool cheat = false;
	bool is_replay = global.replaymode == REPLAY_PLAY;

	switch(type) {
		case EV_PRESS:
			if(global.dialog && (value == KEY_SHOT || value == KEY_BOMB)) {
				useful = page_dialog(&global.dialog);
				break;
			}

			switch(value) {
				case KEY_BOMB:
					useful = player_bomb(plr);

					if(!useful && plr->iddqd) {
						// smooth bomb cancellation test
						player_cancel_bomb(plr, 60);
						useful = true;
					}

					break;

				case KEY_SPECIAL:
					useful = player_powersurge(plr);

					if(!useful/* && plr->iddqd*/) {
						player_cancel_powersurge(plr);
						useful = true;
					}

					break;

				case KEY_IDDQD:
					plr->iddqd = !plr->iddqd;
					cheat = true;
					break;

				case KEY_POWERUP:
					useful = player_add_power(plr,  100);
					cheat = true;
					break;

				case KEY_POWERDOWN:
					useful = player_add_power(plr, -100);
					cheat = true;
					break;

				default:
					useful = player_setinputflag(plr, value, true);
					break;
			}
			break;

		case EV_RELEASE:
			useful = player_setinputflag(plr, value, false);
			break;

		case EV_AXIS_LR:
			useful = player_set_axis(&plr->axis_lr, value);
			break;

		case EV_AXIS_UD:
			useful = player_set_axis(&plr->axis_ud, value);
			break;

		case EV_INFLAGS:
			useful = player_updateinputflags(plr, value);
			break;

		case EV_CONTINUE:
			// continuing in the same frame will desync the replay,
			// so schedule it for the next one
			plr->continuetime = global.frames + 1;
			useful = true;
			break;

		default:
			log_warn("Can not handle event: [%i:%02x:%04x]", global.frames, type, value);
			useful = false;
			break;
	}

	if(is_replay) {
		if(!useful) {
			log_warn("Useless event in replay: [%i:%02x:%04x]", global.frames, type, value);
		}

		if(cheat) {
			log_warn("Cheat event in replay: [%i:%02x:%04x]", global.frames, type, value);

			if( !(global.replay.flags           & REPLAY_GFLAG_CHEATS) ||
				!(global.replay_stage->flags    & REPLAY_SFLAG_CHEATS)) {
				log_warn("...but this replay was NOT properly cheat-flagged! Not cool, not cool at all");
			}
		}

		if(type == EV_CONTINUE && (
			!(global.replay.flags           & REPLAY_GFLAG_CONTINUES) ||
			!(global.replay_stage->flags    & REPLAY_SFLAG_CONTINUES))) {
			log_warn("Continue event in replay: [%i:%02x:%04x], but this replay was not properly continue-flagged", global.frames, type, value);
		}
	}

	if(out_useful) {
		*out_useful = useful;
	}

	if(out_cheat) {
		*out_cheat = cheat;
	}
}

bool player_event_with_replay(Player *plr, uint8_t type, uint16_t value) {
	bool useful, cheat;
	assert(global.replaymode == REPLAY_RECORD);

	if(config_get_int(CONFIG_SHOT_INVERTED) && value == KEY_SHOT && (type == EV_PRESS || type == EV_RELEASE)) {
		type = type == EV_PRESS ? EV_RELEASE : EV_PRESS;
	}

	player_event(plr, type, value, &useful, &cheat);

	if(useful) {
		replay_stage_event(global.replay_stage, global.frames, type, value);

		if(type == EV_CONTINUE) {
			global.replay.flags |= REPLAY_GFLAG_CONTINUES;
			global.replay_stage->flags |= REPLAY_SFLAG_CONTINUES;
		}

		if(cheat) {
			global.replay.flags |= REPLAY_GFLAG_CHEATS;
			global.replay_stage->flags |= REPLAY_SFLAG_CHEATS;
		}

		return true;
	} else {
		log_debug("Useless event discarded: [%i:%02x:%04x]", global.frames, type, value);
	}

	return false;
}

// free-axis movement
static bool player_applymovement_gamepad(Player *plr) {
	if(!plr->axis_lr && !plr->axis_ud) {
		if(plr->gamepadmove) {
			plr->gamepadmove = false;
			plr->inputflags &= ~INFLAGS_MOVE;
		}
		return false;
	}

	complex direction = (
		gamepad_normalize_axis_value(plr->axis_lr) +
		gamepad_normalize_axis_value(plr->axis_ud) * I
	);

	if(cabs(direction) > 1) {
		direction /= cabs(direction);
	}

	int sr = sign(creal(direction));
	int si = sign(cimag(direction));

	player_updateinputflags_moveonly(plr,
		(INFLAG_UP    * (si == -1)) |
		(INFLAG_DOWN  * (si ==  1)) |
		(INFLAG_LEFT  * (sr == -1)) |
		(INFLAG_RIGHT * (sr ==  1))
	);

	if(direction) {
		plr->gamepadmove = true;
		player_move(&global.plr, direction);
	}

	return true;
}

static const char *moveseqname(int dir) {
	switch(dir) {
	case -1: return "left";
	case 0: return "main";
	case 1: return "right";
	default: log_fatal("Invalid player animation dir given");
	}
}

static void player_ani_moving(Player *plr, int dir) {
	if(plr->lastmovesequence == dir)
		return;
	const char *seqname = moveseqname(dir);
	const char *lastseqname = moveseqname(plr->lastmovesequence);

	char *transition = strjoin(lastseqname,"2",seqname,NULL);

	aniplayer_hard_switch(&plr->ani,transition,1);
	aniplayer_queue(&plr->ani,seqname,0);
	plr->lastmovesequence = dir;
	free(transition);
}

void player_applymovement(Player *plr) {
	plr->velocity = 0;

	if(plr->deathtime != -1)
		return;

	bool gamepad = player_applymovement_gamepad(plr);

	int up      =   plr->inputflags & INFLAG_UP;
	int down    =   plr->inputflags & INFLAG_DOWN;
	int left    =   plr->inputflags & INFLAG_LEFT;
	int right   =   plr->inputflags & INFLAG_RIGHT;

	if(left && !right) {
		player_ani_moving(plr,-1);
	} else if(right && !left) {
		player_ani_moving(plr,1);
	} else {
		player_ani_moving(plr,0);
	}

	if(gamepad) {
		return;
	}

	complex direction = 0;

	if(up)      direction -= 1.0*I;
	if(down)    direction += 1.0*I;
	if(left)    direction -= 1.0;
	if(right)   direction += 1.0;

	if(cabs(direction))
		direction /= cabs(direction);

	if(direction)
		player_move(&global.plr, direction);
}

void player_fix_input(Player *plr) {
	// correct input state to account for any events we might have missed,
	// usually because the pause menu ate them up

	PlrInputFlag newflags = plr->inputflags;
	bool invert_shot = config_get_int(CONFIG_SHOT_INVERTED);

	for(KeyIndex key = KEYIDX_FIRST; key <= KEYIDX_LAST; ++key) {
		int flag = key_to_inflag(key);

		if(flag && !(plr->gamepadmove && (flag & INFLAGS_MOVE))) {
			bool flagset = plr->inputflags & flag;
			bool keyheld = gamekeypressed(key);

			if(invert_shot && key == KEY_SHOT) {
				keyheld = !keyheld;
			}

			if(flagset && !keyheld) {
				newflags &= ~flag;
			} else if(!flagset && keyheld) {
				newflags |= flag;
			}
		}
	}

	if(newflags != plr->inputflags) {
		player_event_with_replay(plr, EV_INFLAGS, newflags);
	}

	int axis_lr = gamepad_player_axis_value(PLRAXIS_LR);
	int axis_ud = gamepad_player_axis_value(PLRAXIS_UD);

	if(plr->axis_lr != axis_lr) {
		player_event_with_replay(plr, EV_AXIS_LR, axis_lr);
	}

	if(plr->axis_ud != axis_ud) {
		player_event_with_replay(plr, EV_AXIS_UD, axis_ud);
	}
}

void player_graze(Player *plr, complex pos, int pts, int effect_intensity, const Color *color) {
	if(++plr->graze >= PLR_MAX_GRAZE) {
		log_debug("Graze counter overflow");
		plr->graze = PLR_MAX_GRAZE;
	}

	pos = (pos + plr->pos) * 0.5;

	player_add_points(plr, pts);
	play_sound("graze");

	Color *c = COLOR_COPY(color);
	color_add(c, RGBA(1, 1, 1, 1));
	color_mul_scalar(c, 0.5);
	c->a = 0;

	for(int i = 0; i < effect_intensity; ++i) {
		tsrand_fill(4);

		PARTICLE(
			.sprite = "graze",
			.color = c,
			.pos = pos,
			.rule = asymptotic,
			.timeout = 24 + 5 * afrand(2),
			.draw_rule = ScaleSquaredFade,
			.args = { 0.2 * (1+afrand(0)*5)*cexp(I*M_PI*2*afrand(1)), 16 * (1 + 0.5 * anfrand(3)), 1 },
			.flags = PFLAG_NOREFLECT,
			// .layer = LAYER_PARTICLE_LOW,
		);

		color_mul_scalar(c, 0.4);
	}

	spawn_items(pos, ITEM_POWER_MINI, 1, NULL);
}

static void player_add_fragments(Player *plr, int frags, int *pwhole, int *pfrags, int maxfrags, int maxwhole, const char *fragsnd, const char *upsnd, int score_per_excess) {
	int total_frags = *pfrags + maxfrags * *pwhole;
	int excess_frags = total_frags + frags - maxwhole * maxfrags;

	if(excess_frags > 0) {
		player_add_points(plr, excess_frags * score_per_excess);
		frags -= excess_frags;
	}

	*pfrags += frags;
	int up = *pfrags / maxfrags;

	*pwhole += up;
	*pfrags %= maxfrags;

	if(up) {
		play_sound(upsnd);
	}

	if(frags) {
		// FIXME: when we have the extra life/bomb sounds,
		//        don't play this if upsnd was just played.
		play_sound(fragsnd);
	}

	if(*pwhole >= maxwhole) {
		*pwhole = maxwhole;
		*pfrags = 0;
	}
}

void player_add_life_fragments(Player *plr, int frags) {
	player_add_fragments(
		plr,
		frags,
		&plr->lives,
		&plr->life_fragments,
		PLR_MAX_LIFE_FRAGMENTS,
		PLR_MAX_LIVES,
		"item_generic", // FIXME: replacement needed
		"extra_life",
		(plr->point_item_value * 10) / PLR_MAX_LIFE_FRAGMENTS
	);
}

void player_add_bomb_fragments(Player *plr, int frags) {
	player_add_fragments(
		plr,
		frags,
		&plr->bombs,
		&plr->bomb_fragments,
		PLR_MAX_BOMB_FRAGMENTS,
		PLR_MAX_BOMBS,
		"item_generic",  // FIXME: replacement needed
		"extra_bomb",
		(plr->point_item_value * 5) / PLR_MAX_BOMB_FRAGMENTS
	);
}

void player_add_lives(Player *plr, int lives) {
	player_add_life_fragments(plr, PLR_MAX_LIFE_FRAGMENTS);
}

void player_add_bombs(Player *plr, int bombs) {
	player_add_bomb_fragments(plr, PLR_MAX_BOMB_FRAGMENTS);
}

void player_add_points(Player *plr, uint points) {
	attr_unused uint old = plr->points;
	plr->points += points;

	while(plr->points >= plr->extralife_threshold) {
		player_add_lives(plr, 1);
		plr->extralife_threshold = player_next_extralife_threshold(++plr->extralives_given);
	}
}

void player_add_piv(Player *plr, uint piv) {
	uint v = plr->point_item_value + piv;

	if(v > PLR_MAX_PIV || v < plr->point_item_value) {
		plr->point_item_value = PLR_MAX_PIV;
	} else {
		plr->point_item_value = v;
	}
}

void player_add_voltage(Player *plr, uint voltage) {
	uint v = plr->voltage + voltage;

	if(v > PLR_MAX_VOLTAGE || v < plr->voltage) {
		plr->voltage = PLR_MAX_VOLTAGE;
	} else {
		plr->voltage = v;
	}

	player_add_bomb_fragments(plr, voltage);
}

bool player_drain_voltage(Player *plr, uint voltage) {
	// TODO: animate (or maybe handle that at stagedraw level)

	if(plr->voltage >= voltage) {
		plr->voltage -= voltage;
		return true;
	}

	plr->voltage = 0;
	return false;
}

void player_register_damage(Player *plr, EntityInterface *target, const DamageInfo *damage) {
	if(!DAMAGETYPE_IS_PLAYER(damage->type)) {
		return;
	}

	complex pos = NAN;

	if(target != NULL) {
		switch(target->type) {
			case ENT_ENEMY: {
				player_add_points(&global.plr, damage->amount * 0.5);
				pos = ENT_CAST(target, Enemy)->pos;
				break;
			}

			case ENT_BOSS: {
				player_add_points(&global.plr, damage->amount * 0.2);
				pos = ENT_CAST(target, Boss)->pos;
				break;
			}

			default: break;
		}
	}

	if(!isnan(creal(pos)) && damage->type == DMG_PLAYER_DISCHARGE) {
		double rate = target->type == ENT_BOSS ? 110 : 256;
		spawn_and_collect_items(pos, 1, ITEM_VOLTAGE, (int)(damage->amount / rate), NULL);
	}

	if(player_is_powersurge_active(plr)) {
		plr->powersurge.damage_done += damage->amount;
		plr->powersurge.damage_accum += damage->amount;

		if(!isnan(creal(pos))) {
			double rate = 500;

			while(plr->powersurge.damage_accum > rate) {
				plr->powersurge.damage_accum -= rate;
				spawn_item(pos, ITEM_SURGE);
			}
		}
	}

#ifdef PLR_DPS_STATS
	while(global.frames > plr->dmglogframe) {
		memmove(plr->dmglog + 1, plr->dmglog, sizeof(plr->dmglog) - sizeof(*plr->dmglog));
		plr->dmglog[0] = 0;
		plr->dmglogframe++;
	}

	plr->dmglog[0] += damage->amount;
#endif
}

uint64_t player_next_extralife_threshold(uint64_t step) {
	static uint64_t base = 5000000;
	return base * (step * step + step + 2) / 2;
}

void player_preload(void) {
	const int flags = RESF_DEFAULT;

	preload_resources(RES_SHADER_PROGRAM, flags,
		"player_death",
	NULL);

	preload_resources(RES_TEXTURE, flags,
		"static",
	NULL);

	preload_resources(RES_SPRITE, flags,
		"fairy_circle",
		"focus",
		"part/blast_huge_halo",
		"part/powersurge_field",
	NULL);

	preload_resources(RES_SFX, flags | RESF_OPTIONAL,
		"death",
		"extra_bomb",
		"extra_life",
		"full_power",
		"generic_shot",
		"graze",
		"hit0",
		"hit1",
		"powerup",
	NULL);
}

// FIXME: where should this be?

complex plrutil_homing_target(complex org, complex fallback) {
	double mindst = INFINITY;
	complex target = fallback;

	if(global.boss && boss_is_vulnerable(global.boss)) {
		target = global.boss->pos;
		mindst = cabs(target - org);
	}

	for(Enemy *e = global.enemies.first; e; e = e->next) {
		if(e->hp == ENEMY_IMMUNE) {
			continue;
		}

		double dst = cabs(e->pos - org);

		if(dst < mindst) {
			mindst = dst;
			target = e->pos;
		}
	}

	return target;
}
