/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "stagex.h"
#include "yumemi.h"
#include "draw.h"

#include "renderer/api.h"
#include "util/glm.h"
#include "global.h"

#define BOMBSHIELD_RADIUS 128

DEFINE_ENTITY_TYPE(BossShield, {
	cmplx pos;
	Boss *boss;
	BoxedEnemy hitbox;
	float alpha;

	ShaderProgram *shader;

	Texture *grid_tex;
	Texture *code_tex;
	Texture *shell_tex;

	vec2 grid_aspect;
	vec2 code_aspect;

	struct {
		Uniform *alpha;
		Uniform *time;
		Uniform *code_tex;
		Uniform *grid_tex;
		Uniform *shell_tex;
		Uniform *code_aspect;
		Uniform *grid_aspect;
	} uniforms;
});

static void draw_yumemi_slave(EntityInterface *ent) {
	YumemiSlave *slave = ENT_CAST(ent, YumemiSlave);

	float time = global.frames - slave->spawn_time;
	float s = slave->rotation_factor;
	float opacity;
	float scale;

	if(stagex_drawing_into_glitchmask()) {
		opacity = lerpf(2.0f, 80.0f, slave->glitch_strength);
		scale = 1.1f;
	} else {
		opacity = slave->alpha;
		scale = 1.0f + 6.0f * glm_ease_elast_in(1.0f - slave->alpha);
	}

	scale *= 0.5f;

	SpriteParams sp = {
		.pos.as_cmplx = slave->pos,
		.scale = scale,
		.color = color_mul_scalar(RGBA(1, 1, 1, 0.5), opacity),
	};

	sp.sprite_ptr = slave->sprites.frame;
	sp.rotation.angle = -M_PI/73 * time * s;
	r_draw_sprite(&sp);

	sp.sprite_ptr = slave->sprites.outer;
	sp.rotation.angle = +M_PI/73 * time * s;
	r_draw_sprite(&sp);

	sp.sprite_ptr = slave->sprites.core;
	sp.rotation.angle = 0;
	sp.color = color_mul_scalar(RGBA(0.4, 0.4, 0.4, 0.1), opacity);
	sp.pos.as_cmplx += 3 * cdir(M_PI/72 * time * s);
	r_draw_sprite(&sp);
}

static void bombshield_draw(EntityInterface *ent) {
	BossShield *shield = ENT_CAST(ent, BossShield);
	float alpha = shield->alpha;

	if(alpha <= 0) {
		return;
	}

	float size = BOMBSHIELD_RADIUS * 2 * (2.0 - 1.0 * glm_ease_quad_out(alpha));

	r_shader_ptr(shield->shader);

	r_mat_mv_push();
	r_mat_mv_translate(creal(shield->pos), cimag(shield->pos), 0);

	if(stagex_drawing_into_glitchmask()) {
		float x = rng_f32s() * 16;
		float y = rng_f32s() * 16;
		r_mat_mv_translate(x, y, 0);
	}

	r_mat_mv_scale(size, size, 1);
	r_uniform_float(shield->uniforms.alpha, alpha);
	r_uniform_float(shield->uniforms.time, global.frames / 60.0 * 0.2);
	r_uniform_sampler(shield->uniforms.code_tex, shield->code_tex);
	r_uniform_sampler(shield->uniforms.grid_tex, shield->grid_tex);
	r_uniform_sampler(shield->uniforms.shell_tex, shield->shell_tex);
	r_uniform_vec2_vec(shield->uniforms.code_aspect, shield->code_aspect);
	r_uniform_vec2_vec(shield->uniforms.grid_aspect, shield->grid_aspect);
	r_draw_quad();
	r_mat_mv_pop();
}

static bool is_bombshield_actiev(BossShield *shield) {
	Boss *boss = NOT_NULL(shield->boss);
	return
		boss_is_vulnerable(boss) &&
		player_is_bomb_active(&global.plr) &&
		boss->current && boss->current->type == AT_Spellcard &&
		1;
}

static Enemy *get_bombshield_hitbox(BossShield *shield) {
	Enemy *hitbox = ENT_UNBOX(shield->hitbox);

	if(is_bombshield_actiev(shield)) {
		if(!hitbox) {
			hitbox = create_enemy_p(&global.enemies, 0, 1, NULL, NULL, 0, 0, 0, 0);
			hitbox->flags = (
				EFLAG_IMPENETRABLE |
				EFLAG_INVULNERABLE |
				EFLAG_NO_AUTOKILL |
				EFLAG_NO_DEATH_EXPLOSION |
				EFLAG_NO_HURT |
				EFLAG_NO_VISUAL_CORRECTION |
				0
			);
			hitbox->hit_radius = BOMBSHIELD_RADIUS;
			shield->hitbox = ENT_BOX(hitbox);
		}
	} else {
		if(hitbox) {
			enemy_kill(hitbox);
			hitbox = shield->hitbox.ent = NULL;
		}
	}

	return hitbox;
}

TASK(yumemi_bombshield_expire, { BoxedBossShield shield; }) {
	BossShield *shield = TASK_BIND(ARGS.shield);
	Enemy *hitbox = ENT_UNBOX(shield->hitbox);

	if(hitbox) {
		enemy_kill(hitbox);
		shield->hitbox.ent = NULL;
	}
}

TASK(yumemi_bombshield_controller, { BoxedBoss boss; }) {
	Boss *boss = TASK_BIND(ARGS.boss);
	BossShield *shield = TASK_HOST_ENT(BossShield);

	shield->boss = boss;
	shield->ent.draw_func = bombshield_draw;
	shield->ent.draw_layer = LAYER_BOSS | 0x10;

	shield->shader = res_shader("bombshield");
	shield->grid_tex = res_texture("stagex/hex_tiles");
	shield->code_tex = res_texture("stagex/bg_binary");
	shield->shell_tex = res_texture("stagex/bg");

	uint w, h;

	r_texture_get_size(shield->grid_tex, 0, &w, &h);
	shield->grid_aspect[0] = 512.0f / (float)w;
	shield->grid_aspect[1] = 512.0f / (float)h;

	r_texture_get_size(shield->code_tex, 0, &w, &h);
	shield->code_aspect[0] = 256.0f / (float)w;
	shield->code_aspect[1] = 256.0f / (float)h;

	shield->uniforms.alpha = r_shader_uniform(shield->shader, "alpha");
	shield->uniforms.time = r_shader_uniform(shield->shader, "time");
	shield->uniforms.code_tex = r_shader_uniform(shield->shader, "code_tex");
	shield->uniforms.grid_tex = r_shader_uniform(shield->shader, "grid_tex");
	shield->uniforms.shell_tex = r_shader_uniform(shield->shader, "shell_tex");
	shield->uniforms.code_aspect = r_shader_uniform(shield->shader, "code_aspect");
	shield->uniforms.grid_aspect = r_shader_uniform(shield->shader, "grid_aspect");

	INVOKE_TASK_AFTER(&TASK_EVENTS(THIS_TASK)->finished, yumemi_bombshield_expire, ENT_BOX(shield));

	for(;;YIELD) {
		shield->pos = boss->pos;
		Enemy *hitbox = get_bombshield_hitbox(shield);

		if(hitbox) {
			hitbox->pos = shield->pos;
			boss->bomb_damage_multiplier = 0;
			boss->shot_damage_multiplier = 0;
			fapproach_p(&shield->alpha, 1.0f, 1.0/30.0f);
		} else {
			boss->bomb_damage_multiplier = 1;
			boss->shot_damage_multiplier = 1;
			fapproach_p(&shield->alpha, 0.0f, 1.0/15.0f);
		}
	}
}

Boss *stagex_spawn_yumemi(cmplx pos) {
	Boss *yumemi = create_boss("Okazaki Yumemi", "yumemi", pos - 400 * I);
	boss_set_portrait(yumemi, "yumemi", NULL, "normal");
	yumemi->shadowcolor = *RGBA(0.5, 0.0, 0.22, 1);
	yumemi->glowcolor = *RGBA(0.30, 0.0, 0.12, 0);
	yumemi->move = move_towards(pos, 0.01);
	yumemi->pos = pos;

	INVOKE_TASK(yumemi_bombshield_controller, ENT_BOX(yumemi));

	return yumemi;
}

void stagex_init_yumemi_slave(YumemiSlave *slave, cmplx pos, int type) {
	slave->pos = pos;
	slave->ent.draw_func = draw_yumemi_slave;
	slave->ent.draw_layer = LAYER_BOSS - 1;
	slave->spawn_time = global.frames;

	switch(type) {
		case 0:
			slave->sprites.core = res_sprite("stagex/yumemi_slaves/zero_core");
			slave->sprites.frame = res_sprite("stagex/yumemi_slaves/zero_frame");
			slave->sprites.outer = res_sprite("stagex/yumemi_slaves/zero_outer");
			slave->rotation_factor = 1;
			break;

		case 1:
			slave->sprites.core = res_sprite("stagex/yumemi_slaves/one_core");
			slave->sprites.frame = res_sprite("stagex/yumemi_slaves/one_frame");
			slave->sprites.outer = res_sprite("stagex/yumemi_slaves/one_outer");
			slave->rotation_factor = -1;
			break;

		default: UNREACHABLE;
	}
}

TASK(yumemi_slave_fadein, { BoxedYumemiSlave slave; }) {
	YumemiSlave *slave = TASK_BIND(ARGS.slave);

	slave->alpha = 0;
	slave->glitch_strength = 1;

	while(
		fapproach_p(&slave->alpha,           1.0f, 1.0f/60.0f) < 0.0f ||
		fapproach_p(&slave->glitch_strength, 0.0f, 1.0f/80.0f) > 0.0f
	) {
		YIELD;
	}
}

TASK(yumemi_slave_fader, { BoxedYumemiSlave slave; }) {
	YumemiSlave *slave = NOT_NULL(ENT_UNBOX(ARGS.slave));
	YumemiSlave *fader = TASK_HOST_ENT(YumemiSlave);

	fader->ent.draw_func = draw_yumemi_slave;
	fader->ent.draw_layer = LAYER_BOSS - 1;
	fader->sprites = slave->sprites;
	fader->spawn_time = slave->spawn_time;
	fader->rotation_factor = slave->rotation_factor;
	fader->alpha = slave->alpha;
	fader->pos = slave->pos;
	fader->glitch_strength = 1;

	slave = NULL;

	while(fapproach_p(&fader->alpha, 0.0f, 1.0f / 60.0f) > 0) {
		YIELD;
	}
}

YumemiSlave *stagex_host_yumemi_slave(cmplx pos, int type) {
	YumemiSlave *slave = TASK_HOST_ENT(YumemiSlave);
	TASK_HOST_EVENTS(slave->events);
	stagex_init_yumemi_slave(slave, pos, type);
	INVOKE_TASK(yumemi_slave_fadein, ENT_BOX(slave));
	INVOKE_TASK_AFTER(&slave->events.despawned, yumemi_slave_fader, ENT_BOX(slave));
	return slave;
}

void stagex_despawn_yumemi_slave(YumemiSlave *slave) {
	coevent_signal_once(&slave->events.despawned);
}

void stagex_draw_yumemi_portrait_overlay(SpriteParams *sp) {
	StageXDrawData *draw_data = stagex_get_draw_data();

	sp->sprite = NULL;
	sp->sprite_ptr = res_sprite("dialog/yumemi_misc_code_mask");
	sp->shader = NULL;
	sp->shader_ptr = res_shader("sprite_yumemi_overlay");
	sp->aux_textures[0] = res_texture("stagex/code");
	sp->shader_params = &(ShaderCustomParams) {
		global.frames / 60.0,
		draw_data->codetex_aspect[0],
		draw_data->codetex_aspect[1],
		draw_data->codetex_num_segments,
	};

	r_draw_sprite(sp);
}

static void render_spellbg_mask(Framebuffer *fb) {
	r_state_push();
	r_framebuffer(fb);
	r_blend(BLEND_NONE);
	r_clear(CLEAR_COLOR, RGBA(0, 0, 0, 1), 1);
	r_shader("yumemi_spellbg_voronoi");
	r_mat_mv_push();
	r_mat_mv_scale(VIEWPORT_W, VIEWPORT_H, 1);
	r_mat_mv_translate(0.5, 0.5, 0);
	r_uniform_float("time", global.frames / 60.0f);
	r_uniform_vec4("color", 0.1, 0.2, 0.05, 1.0);
	r_draw_quad();
	r_mat_mv_pop();
	r_state_pop();
}

void stagex_draw_yumemi_spellbg_voronoi(Boss *boss, int time) {
	StageXDrawData *draw_data = stagex_get_draw_data();
	render_spellbg_mask(draw_data->fb.spell_background_lq);

	r_state_push();
	r_blend(BLEND_NONE);
	r_clear(CLEAR_COLOR, RGBA(0, 0, 0, 1), 1);
	r_shader("yumemi_spellbg_voronoi_compose");
	r_uniform_sampler("tex2", r_framebuffer_get_attachment(draw_data->fb.spell_background_lq, FRAMEBUFFER_ATTACH_COLOR0));
	// draw_framebuffer_tex(draw_data->fb.spell_background_lq, VIEWPORT_W, VIEWPORT_H);
	fill_viewport(0, time/700.0+0.5, 0, "stagex/bg");
	r_state_pop();
}
