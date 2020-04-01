/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "global.h"
#include "plrmodes.h"
#include "marisa.h"
#include "renderer/api.h"
#include "stagedraw.h"

#define SHOT_FORWARD_DAMAGE 100
#define SHOT_FORWARD_DELAY 6
#define SHOT_LASER_DAMAGE 10

typedef struct MarisaAController MarisaAController;
typedef struct MarisaLaser MarisaLaser;
typedef struct MarisaSlave MarisaSlave;
typedef struct MasterSpark MasterSpark;

struct MarisaSlave {
	ENTITY_INTERFACE_NAMED(MarisaAController, ent);
	Sprite *sprite;
	ShaderProgram *shader;
	cmplx pos;
	real lean;
	real shot_angle;
	real flare_alpha;
	bool alive;
};

struct MarisaLaser {
	LIST_INTERFACE(MarisaLaser);
	cmplx pos;
	struct {
		cmplx first;
		cmplx last;
	} trace_hit;
	real alpha;
};

struct MasterSpark {
	ENTITY_INTERFACE_NAMED(MarisaAController, ent);
	cmplx pos;
	cmplx dir;
	real alpha;
};

struct MarisaAController {
	ENTITY_INTERFACE_NAMED(MarisaAController, laser_renderer);

	Player *plr;
	LIST_ANCHOR(MarisaLaser) lasers;

	COEVENTS_ARRAY(
		slaves_expired
	) events;
};

static void trace_laser(MarisaLaser *laser, cmplx vel, real damage) {
	ProjCollisionResult col;
	ProjectileList lproj = { .first = NULL };

	PROJECTILE(
		.dest = &lproj,
		.pos = laser->pos,
		.size = 28*(1+I),
		.type = PROJ_PLAYER,
		.damage = damage,
		.rule = linear,
		.args = { vel },
	);

	bool first_found = false;
	int timeofs = 0;
	int col_types = PCOL_ENTITY;

	struct enemy_col {
		Enemy *enemy;
		int original_hp;
	} enemy_collisions[64] = { 0 };  // 64 collisions ought to be enough for everyone

	int num_enemy_collissions = 0;

	while(lproj.first) {
		timeofs = trace_projectile(lproj.first, &col, col_types | PCOL_VOID, timeofs);
		struct enemy_col *c = NULL;

		if(!first_found) {
			laser->trace_hit.first = col.location;
			first_found = true;
		}

		if(col.type & col_types) {
			RNG_ARRAY(R, 3);
			PARTICLE(
				.sprite = "flare",
				.pos = col.location,
				.rule = linear,
				.timeout = vrng_range(R[0], 3, 8),
				.draw_rule = pdraw_timeout_scale(2, 0.01),
				.args = { vrng_range(R[1], 2, 8) * vrng_dir(R[2]) },
				.flags = PFLAG_NOREFLECT,
				.layer = LAYER_PARTICLE_HIGH,
			);

			if(col.type == PCOL_ENTITY && col.entity->type == ENT_ENEMY) {
				assert(num_enemy_collissions < ARRAY_SIZE(enemy_collisions));
				if(num_enemy_collissions < ARRAY_SIZE(enemy_collisions)) {
					c = enemy_collisions + num_enemy_collissions++;
					c->enemy = ENT_CAST(col.entity, Enemy);
				}
			} else {
				col_types &= ~col.type;
			}

			col.fatal = false;
		}

		apply_projectile_collision(&lproj, lproj.first, &col);

		if(c) {
			c->original_hp = (ENT_CAST(col.entity, Enemy))->hp;
			(ENT_CAST(col.entity, Enemy))->hp = ENEMY_IMMUNE;
		}
	}

	assume(num_enemy_collissions < ARRAY_SIZE(enemy_collisions));

	for(int i = 0; i < num_enemy_collissions; ++i) {
		enemy_collisions[i].enemy->hp = enemy_collisions[i].original_hp;
	}

	laser->trace_hit.last = col.location;
}

static float set_alpha(Uniform *u_alpha, float a) {
	if(a) {
		r_uniform_float(u_alpha, a);
	}

	return a;
}

static float set_alpha_dimmed(Uniform *u_alpha, float a) {
	return set_alpha(u_alpha, a * a * 0.5);
}

static void marisa_laser_draw_slave(EntityInterface *ent) {
	MarisaSlave *slave = ENT_CAST_CUSTOM(ent, MarisaSlave);

	ShaderCustomParams shader_params;
	shader_params.color = *RGBA(0.2, 0.4, 0.5, slave->flare_alpha * 0.75);
	float t = global.frames;

	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = slave->sprite,
		.shader_ptr = slave->shader,
		.pos.as_cmplx = slave->pos,
		.rotation.angle = t * 0.05f,
		.color = color_lerp(RGB(0.2, 0.4, 0.5), RGB(1.0, 1.0, 1.0), 0.25 * powf(psinf(t / 6.0f), 2.0f) * slave->flare_alpha),
		.shader_params = &shader_params,
	});
}

static void draw_laser_beam(cmplx src, cmplx dst, real size, real step, real t, Texture *tex, Uniform *u_length) {
	cmplx dir = dst - src;
	cmplx center = (src + dst) * 0.5;

	r_mat_mv_push();

	r_mat_mv_translate(creal(center), cimag(center), 0);
	r_mat_mv_rotate(carg(dir), 0, 0, 1);
	r_mat_mv_scale(cabs(dir), size, 1);

	r_mat_tex_push_identity();
	r_mat_tex_translate(-cimag(src) / step + t, 0, 0);
	r_mat_tex_scale(cabs(dir) / step, 1, 1);

	r_uniform_sampler("tex", tex);
	r_uniform_float(u_length, cabs(dir) / step);
	r_draw_quad();

	r_mat_tex_pop();
	r_mat_mv_pop();
}

static void marisa_laser_draw_lasers(EntityInterface *ent) {
	MarisaAController *ctrl = ENT_CAST_CUSTOM(ent, MarisaAController);
	real t = global.frames;

	ShaderProgram *shader = r_shader_get("marisa_laser");
	Uniform *u_clr0 = r_shader_uniform(shader, "color0");
	Uniform *u_clr1 = r_shader_uniform(shader, "color1");
	Uniform *u_clr_phase = r_shader_uniform(shader, "color_phase");
	Uniform *u_clr_freq = r_shader_uniform(shader, "color_freq");
	Uniform *u_alpha = r_shader_uniform(shader, "alphamod");
	Uniform *u_length = r_shader_uniform(shader, "laser_length");
	Texture *tex0 = r_texture_get("part/marisa_laser0");
	Texture *tex1 = r_texture_get("part/marisa_laser1");
	FBPair *fbp_aux = stage_get_fbpair(FBPAIR_FG_AUX);
	Framebuffer *target_fb = r_framebuffer_current();

	r_shader_ptr(shader);
	r_uniform_vec4(u_clr0,       0.5, 0.5, 0.5, 0.0);
	r_uniform_vec4(u_clr1,       0.8, 0.8, 0.8, 0.0);
	r_uniform_float(u_clr_phase, -1.5 * t/M_PI);
	r_uniform_float(u_clr_freq,  10.0);
	r_framebuffer(fbp_aux->back);
	r_clear(CLEAR_COLOR, RGBA(0, 0, 0, 0), 1);
	r_color4(1, 1, 1, 1);

	r_blend(r_blend_compose(
		BLENDFACTOR_SRC_COLOR, BLENDFACTOR_ONE, BLENDOP_MAX,
		BLENDFACTOR_SRC_COLOR, BLENDFACTOR_ONE, BLENDOP_MAX
	));

	for(MarisaLaser *laser = ctrl->lasers.first; laser; laser = laser->next) {
		if(set_alpha(u_alpha, laser->alpha)) {
			draw_laser_beam(laser->pos, laser->trace_hit.last, 32, 128, -0.02 * t, tex1, u_length);
		}
	}

	r_blend(BLEND_PREMUL_ALPHA);
	fbpair_swap(fbp_aux);

	stage_draw_begin_noshake();

	r_framebuffer(fbp_aux->back);
	r_clear(CLEAR_COLOR, RGBA(0, 0, 0, 0), 1);
	r_shader("max_to_alpha");
	draw_framebuffer_tex(fbp_aux->front, VIEWPORT_W, VIEWPORT_H);
	fbpair_swap(fbp_aux);

	r_framebuffer(target_fb);
	r_shader_standard();
	r_color4(1, 1, 1, 1);
	draw_framebuffer_tex(fbp_aux->front, VIEWPORT_W, VIEWPORT_H);
	r_shader_ptr(shader);

	stage_draw_end_noshake();

	r_uniform_vec4(u_clr0, 0.5, 0.0, 0.0, 0.0);
	r_uniform_vec4(u_clr1, 1.0, 0.0, 0.0, 0.0);

	for(MarisaLaser *laser = ctrl->lasers.first; laser; laser = laser->next) {
		if(set_alpha_dimmed(u_alpha, laser->alpha)) {
			draw_laser_beam(laser->pos, laser->trace_hit.first, 40, 128, t * -0.12, tex0, u_length);
		}
	}

	r_uniform_vec4(u_clr0, 2.0, 1.0, 2.0, 0.0);
	r_uniform_vec4(u_clr1, 0.1, 0.1, 1.0, 0.0);

	for(MarisaLaser *laser = ctrl->lasers.first; laser; laser = laser->next) {
		if(set_alpha_dimmed(u_alpha, laser->alpha)) {
			draw_laser_beam(laser->pos, laser->trace_hit.first, 42, 200, t * -0.03, tex0, u_length);
		}
	}

	SpriteParams sp = {
		.sprite_ptr = get_sprite("part/smoothdot"),
		.shader_ptr = r_shader_get("sprite_default"),
	};

	for(MarisaLaser *laser = ctrl->lasers.first; laser; laser = laser->next) {
		float a = laser->alpha * 0.8f;
		cmplx spread;

		sp.pos.as_cmplx = laser->trace_hit.first;

		spread = rng_dir(); spread *= rng_range(0, 3);
		sp.pos.as_cmplx += spread;
		sp.color = color_mul_scalar(RGBA(0.8, 0.3, 0.5, 0), a);
		r_draw_sprite(&sp);

		spread = rng_dir(); spread *= rng_range(0, 3);
		sp.pos.as_cmplx += spread;
		sp.color = color_mul_scalar(RGBA(0.2, 0.7, 0.5, 0), a);
		r_draw_sprite(&sp);
	}
}

static void marisa_laser_flash_draw(Projectile *p, int t, ProjDrawRuleArgs args) {
	SpriteParamsBuffer spbuf;
	SpriteParams sp = projectile_sprite_params(p, &spbuf);
	float o = 1 - t / p->timeout;
	color_mul_scalar(&spbuf.color, o);
	spbuf.color.r *= o;
	r_draw_sprite(&sp);
}

TASK(marisa_laser_slave_cleanup, { BoxedEntity slave; }) {
	MarisaSlave *slave = TASK_BIND_CUSTOM(ARGS.slave, MarisaSlave);
	ent_unregister(&slave->ent);
}

TASK(marisa_laser_slave_expire, { BoxedEntity slave; }) {
	MarisaSlave *slave = TASK_BIND_CUSTOM(ARGS.slave, MarisaSlave);
	slave->alive = false;
}

static void marisa_laser_show_laser(MarisaAController *ctrl, MarisaLaser *laser) {
	alist_append(&ctrl->lasers, laser);
}

static void marisa_laser_hide_laser(MarisaAController *ctrl, MarisaLaser *laser) {
	alist_unlink(&ctrl->lasers, laser);
}

TASK(marisa_laser_fader, {
	MarisaAController *ctrl;
	const MarisaLaser *ref_laser;
}) {
	MarisaAController *ctrl = ARGS.ctrl;
	MarisaLaser fader = *ARGS.ref_laser;

	marisa_laser_show_laser(ctrl, &fader);

	while(fader.alpha > 0) {
		YIELD;
		approach_p(&fader.alpha, 0, 0.1);
	}

	marisa_laser_hide_laser(ctrl, &fader);
}

static void marisa_laser_fade_laser(MarisaAController *ctrl, MarisaLaser *laser) {
	marisa_laser_hide_laser(ctrl, laser);
	INVOKE_TASK(marisa_laser_fader, ctrl, laser);
}

TASK(marisa_laser_slave_shot_cleanup, {
	MarisaAController *ctrl;
	MarisaLaser **active_laser;
}) {
	MarisaLaser *active_laser = *ARGS.active_laser;
	if(active_laser) {
		marisa_laser_fade_laser(ARGS.ctrl, active_laser);
	}
}

TASK(marisa_laser_slave_shot, {
	MarisaAController *ctrl;
	BoxedEntity slave;
}) {
	MarisaAController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;
	MarisaSlave *slave = TASK_BIND_CUSTOM(ARGS.slave, MarisaSlave);

	Animation *fire_anim = get_ani("fire");
	AniSequence *fire_anim_seq = get_ani_sequence(fire_anim, "main");

	MarisaLaser *active_laser = NULL;
	INVOKE_TASK_AFTER(&TASK_EVENTS(THIS_TASK)->finished, marisa_laser_slave_shot_cleanup, ctrl, &active_laser);

	for(;;) {
		WAIT_EVENT_OR_DIE(&plr->events.shoot);
		assert(player_should_shoot(plr));

		// We started shooting - register a laser for rendering

		MarisaLaser laser = { 0 };
		marisa_laser_show_laser(ctrl, &laser);
		active_laser = &laser;

		while(player_should_shoot(plr)) {
			real angle = slave->shot_angle * (1.0 + 0.7 * psin(global.frames/15.0));

			laser.pos = slave->pos;
			approach_p(&laser.alpha, 1.0, 0.2);

			cmplx dir = -cdir(angle + slave->lean + M_PI/2);
			trace_laser(&laser, 5 * dir, SHOT_LASER_DAMAGE);

			Sprite *spr = animation_get_frame(fire_anim, fire_anim_seq, global.frames);

			PARTICLE(
				.sprite_ptr = spr,
				.size = 1+I,
				.pos = laser.pos + dir * 10,
				.color = color_mul_scalar(RGBA(2, 0.2, 0.5, 0), 0.2),
				.rule = linear,
				.draw_rule = marisa_laser_flash_draw,
				.timeout = 8,
				.args = { dir, 0, 0.6 + 0.2*I, },
				.flags = PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE,
				.scale = 0.4,
			);

			YIELD;
		}

		// We stopped shooting - remove the laser, spawn fader

		marisa_laser_fade_laser(ctrl, &laser);
		active_laser = NULL;
	}
}

TASK(marisa_laser_slave, {
	MarisaAController *ctrl;
	cmplx unfocused_offset;
	cmplx focused_offset;
	real shot_angle;
}) {
	MarisaAController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;
	MarisaSlave slave = { 0 };
	slave.shader = r_shader_get("sprite_hakkero");
	slave.sprite = get_sprite("hakkero");
	slave.ent.draw_layer = LAYER_PLAYER_SLAVE;
	slave.ent.draw_func = marisa_laser_draw_slave;
	slave.pos = plr->pos;
	slave.alive = true;
	ent_register(&slave.ent, ENT_CUSTOM);

	INVOKE_TASK_AFTER(&TASK_EVENTS(THIS_TASK)->finished, marisa_laser_slave_cleanup, ENT_BOX_CUSTOM(&slave));
	INVOKE_TASK_WHEN(&ctrl->events.slaves_expired, marisa_laser_slave_expire, ENT_BOX_CUSTOM(&slave));

	BoxedTask shot_task = cotask_box(INVOKE_SUBTASK(marisa_laser_slave_shot,
		.ctrl = ctrl,
		.slave = ENT_BOX_CUSTOM(&slave)
	));

	/*                     unfocused              focused              */
	cmplx offsets[]    = { ARGS.unfocused_offset, ARGS.focused_offset, };
	real shot_angles[] = { ARGS.shot_angle,       0                    };

	real formation_switch_rate = 0.3;
	real lean_rate = 0.1;
	real follow_rate = 0.5;
	real lean_strength = 0.01;
	real epsilon = 1e-5;

	cmplx offset = 0;
	cmplx prev_pos = slave.pos;

	while(slave.alive) {
		bool focused = plr->inputflags & INFLAG_FOCUS;

		approach_asymptotic_p(&slave.shot_angle, shot_angles[focused], formation_switch_rate, epsilon);
		capproach_asymptotic_p(&offset, offsets[focused], formation_switch_rate, epsilon);

		capproach_asymptotic_p(&slave.pos, plr->pos + offset, follow_rate, epsilon);
		approach_asymptotic_p(&slave.lean, -lean_strength * creal(slave.pos - prev_pos), lean_rate, epsilon);
		prev_pos = slave.pos;

		if(player_should_shoot(plr)) {
			approach_asymptotic_p(&slave.flare_alpha, 1.0, 0.2, epsilon);
		} else {
			approach_asymptotic_p(&slave.flare_alpha, 0.0, 0.08, epsilon);
		}

		YIELD;
	}

	CoTask *t = cotask_unbox(shot_task);
	if(t) {
		cotask_cancel(t);
	}

	real retract_time = 4;
	cmplx pos0 = slave.pos;

	for(int i = 1; i <= retract_time; ++i) {
		slave.pos = clerp(pos0, plr->pos, i / retract_time);
		YIELD;
	}

	ent_unregister(&slave.ent);
}

static real marisa_laser_masterspark_width(real progress) {
	real w = 1;

	if(progress < 1./6) {
		w = progress * 6;
		w = pow(w, 1.0/3.0);
	}

	if(progress > 4./5) {
		w = 1 - progress * 5 + 4;
		w = pow(w, 5);
	}

	return w;
}

static void marisa_laser_draw_masterspark(EntityInterface *ent) {
	MasterSpark *ms = ENT_CAST_CUSTOM(ent, MasterSpark);
	marisa_common_masterspark_draw(1, &(MarisaBeamInfo) {
		ms->pos,
		800 + I * VIEWPORT_H * 1.25,
		carg(ms->dir),
		global.frames
	}, ms->alpha);
}

static void marisa_laser_masterspark_damage(MasterSpark *ms) {
	// lazy inefficient approximation of the beam parabola

	float r = 96 * ms->alpha;
	float growth = 0.25;
	cmplx v = ms->dir * cdir(M_PI * -0.5);
	cmplx p = ms->pos + v * r;

	Rect vp_rect, seg_rect;
	vp_rect.top_left = 0;
	vp_rect.bottom_right = CMPLX(VIEWPORT_W, VIEWPORT_H);

	int iter = 0;
	int maxiter = 10;

	do {
		stage_clear_hazards_at(p, r, CLEAR_HAZARDS_ALL | CLEAR_HAZARDS_NOW);
		ent_area_damage(p, r, &(DamageInfo) { 80, DMG_PLAYER_BOMB }, NULL, NULL);

		p += v * r;
		r *= 1 + growth;
		growth *= 0.75;

		cmplx o = (1 + I);
		seg_rect.top_left = p - o * r;
		seg_rect.bottom_right = p + o * r;

		++iter;
	} while(rect_rect_intersect(seg_rect, vp_rect, true, true) && iter < maxiter);
	// log_debug("%i", iter);
}

TASK(marisa_laser_masterspark_cleanup, { BoxedEntity ms; }) {
	MasterSpark *ms = TASK_BIND_CUSTOM(ARGS.ms, MasterSpark);
	ent_unregister(&ms->ent);
}

TASK(marisa_laser_masterspark, { MarisaAController *ctrl; }) {
	MarisaAController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;

	MasterSpark ms = { 0 };
	ms.dir = 1;
	ms.ent.draw_func = marisa_laser_draw_masterspark;
	ms.ent.draw_layer = LAYER_PLAYER_FOCUS - 1;
	ent_register(&ms.ent, ENT_CUSTOM);

	INVOKE_TASK_AFTER(&TASK_EVENTS(THIS_TASK)->finished, marisa_laser_masterspark_cleanup, ENT_BOX_CUSTOM(&ms));

	int t = 0;

	Sprite *star_spr = get_sprite("part/maristar_orbit");
	Sprite *smoke_spr = get_sprite("part/smoke");

	do {
		real bomb_progress = player_get_bomb_progress(plr);
		ms.alpha = marisa_laser_masterspark_width(bomb_progress);
		ms.dir *= cdir(0.005 * (creal(plr->velocity) + 2 * rng_sreal()));
		ms.pos = plr->pos - 30 * I;

		marisa_laser_masterspark_damage(&ms);

		if(bomb_progress >= 3.0/4.0) {
			global.shake_view = 8 * (1 - bomb_progress * 4 + 3);
			goto skip_particles;
		}

		global.shake_view = 8;

		uint pflags = PFLAG_NOREFLECT | PFLAG_MANUALANGLE;

		if(t % 4 == 0) {
			pflags |= PFLAG_REQUIREDPARTICLE;
		}

		cmplx dir = -cdir(1.5 * sin(t * M_PI * 1.12)) * I;
		Color *c = HSLA(-bomb_progress * 5.321, 1, 0.5, rng_range(0, 0.5));
		cmplx pos = plr->pos + 40 * dir;

		for(int i = 0; i < 2; ++i) {
			cmplx v = 10 * (dir - I);
			PARTICLE(
				.angle = rng_angle(),
				.angle_delta = 0.1,
				.color = c,
				.draw_rule = pdraw_timeout_scalefade(0, 5, 1, 0),
				.flags = pflags,
				.move = move_accelerated(v, 0.1 * cnormalize(v)),
				.pos = pos,
				.sprite_ptr = star_spr,
				.timeout = 50,
			);
			dir = -conj(dir);
		}

		PARTICLE(
			.sprite_ptr = smoke_spr,
			.pos = plr->pos - 40*I,
			.color = HSLA(2 * bomb_progress, 1, 2, 0),
			.timeout = 50,
			.move = move_linear(7 * (I - dir)),
			.angle = rng_angle(),
			.draw_rule = pdraw_timeout_scalefade(0, 7, 1, 0),
			.flags = pflags,
		);

skip_particles:
		++t;
		YIELD;
	} while(player_is_bomb_active(plr));

	global.shake_view = 0;

	while(ms.alpha > 0) {
		approach_p(&ms.alpha, 0, 0.2);
		YIELD;
	}

	ent_unregister(&ms.ent);
}

TASK(marisa_laser_bomb_background, { MarisaAController *ctrl; }) {
	MarisaAController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;
	CoEvent *draw_event = &stage_get_draw_events()->background_drawn;

	do {
		WAIT_EVENT_OR_DIE(draw_event);
		float t = player_get_bomb_progress(plr);
		float fade = 1;

		if(t < 1./6) {
			fade = t*6;
		}

		if(t > 3./4) {
			fade = 1-t*4 + 3;
		}

		r_color4(0.8 * fade, 0.8 * fade, 0.8 * fade, 0.8 * fade);
		fill_viewport(sin(t * 0.3), t * 3 * (1 + t * 3), 1, "marisa_bombbg");
		r_color4(1, 1, 1, 1);
	} while(player_is_bomb_active(plr));
}

TASK(marisa_laser_bomb_handler, { MarisaAController *ctrl; }) {
	MarisaAController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;

	for(;;) {
		WAIT_EVENT_OR_DIE(&plr->events.bomb_used);
		play_sound("bomb_marisa_a");
		INVOKE_SUBTASK(marisa_laser_bomb_background, ctrl);
		INVOKE_SUBTASK(marisa_laser_masterspark, ctrl);
	}
}

static void marisa_laser_respawn_slaves(MarisaAController *ctrl, int power_rank) {
	coevent_signal(&ctrl->events.slaves_expired);

	if(power_rank == 1) {
		INVOKE_TASK(marisa_laser_slave, ctrl, -40.0*I, -40.0*I, 0);
	}

	if(power_rank >= 2) {
		INVOKE_TASK(marisa_laser_slave, ctrl,  25-5.0*I,  9-40.0*I,  M_PI/30);
		INVOKE_TASK(marisa_laser_slave, ctrl, -25-5.0*I, -9-40.0*I, -M_PI/30);
	}

	if(power_rank == 3) {
		INVOKE_TASK(marisa_laser_slave, ctrl, -30.0*I, -55.0*I, 0);
	}

	if(power_rank >= 4) {
		INVOKE_TASK(marisa_laser_slave, ctrl,  17-30.0*I,  18-55.0*I,  M_PI/60);
		INVOKE_TASK(marisa_laser_slave, ctrl, -17-30.0*I, -18-55.0*I, -M_PI/60);
	}
}

TASK(marisa_laser_power_handler, { MarisaAController *ctrl; }) {
	MarisaAController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;
	int old_power = plr->power / 100;

	marisa_laser_respawn_slaves(ctrl, old_power);

	for(;;) {
		WAIT_EVENT_OR_DIE(&plr->events.power_changed);
		int new_power = plr->power / 100;
		if(old_power != new_power) {
			marisa_laser_respawn_slaves(ctrl, new_power);
			old_power = new_power;
		}
	}
}

TASK(marisa_laser_shot_forward, { MarisaAController *ctrl; }) {
	MarisaAController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;
	ShaderProgram *bolt_shader = r_shader_get("sprite_particle");

	for(;;) {
		WAIT_EVENT_OR_DIE(&plr->events.shoot);
		play_loop("generic_shot");

		for(int i = -1; i < 2; i += 2) {
			PROJECTILE(
				.proto = pp_marisa,
				.pos = plr->pos + 10 * i - 15.0*I,
				.move = move_linear(-20*I),
				.type = PROJ_PLAYER,
				.damage = SHOT_FORWARD_DAMAGE,
				.shader_ptr = bolt_shader,
			);
		}

		WAIT(SHOT_FORWARD_DELAY);
	}
}

TASK(marisa_laser_controller, { BoxedPlayer plr; }) {
	MarisaAController ctrl = { 0 };
	ctrl.plr = TASK_BIND(ARGS.plr);
	COEVENT_INIT_ARRAY(ctrl.events);

	ctrl.laser_renderer.draw_func = marisa_laser_draw_lasers;
	ctrl.laser_renderer.draw_layer = LAYER_PLAYER_FOCUS;
	ent_register(&ctrl.laser_renderer, ENT_CUSTOM);

	INVOKE_SUBTASK(marisa_laser_power_handler, &ctrl);
	INVOKE_SUBTASK(marisa_laser_bomb_handler, &ctrl);
	INVOKE_SUBTASK(marisa_laser_shot_forward, &ctrl);

	WAIT_EVENT(&TASK_EVENTS(THIS_TASK)->finished);
	COEVENT_CANCEL_ARRAY(ctrl.events);
	ent_unregister(&ctrl.laser_renderer);
}

static void marisa_laser_init(Player *plr) {
	INVOKE_TASK(marisa_laser_controller, ENT_BOX(plr));
}

static double marisa_laser_property(Player *plr, PlrProperty prop) {
	switch(prop) {
		case PLR_PROP_SPEED: {
			double s = marisa_common_property(plr, prop);

			if(player_is_bomb_active(plr)) {
				s /= 5.0;
			}

			return s;
		}

		default:
			return marisa_common_property(plr, prop);
	}
}

static void marisa_laser_preload(void) {
	const int flags = RESF_DEFAULT;

	preload_resources(RES_SPRITE, flags,
		"proj/marisa",
		"part/maristar_orbit",
		"hakkero",
		"masterspark_ring",
	NULL);

	preload_resources(RES_TEXTURE, flags,
		"marisa_bombbg",
		"part/marisa_laser0",
		"part/marisa_laser1",
	NULL);

	preload_resources(RES_SHADER_PROGRAM, flags,
		"blur25",
		"blur5",
		"marisa_laser",
		"masterspark",
		"max_to_alpha",
		"sprite_hakkero",
	NULL);

	preload_resources(RES_ANIM, flags,
		"fire",
	NULL);

	preload_resources(RES_SFX, flags | RESF_OPTIONAL,
		"bomb_marisa_a",
	NULL);
}

PlayerMode plrmode_marisa_a = {
	.name = "Illusion Laser",
	.description = "Magic missiles and lasers — simple and to the point. They’ve never let you down before, so why stop now?",
	.spellcard_name = "Pure Love “Galactic Spark”",
	.character = &character_marisa,
	.dialog = &dialog_tasks_marisa,
	.shot_mode = PLR_SHOT_MARISA_LASER,
	.procs = {
		.init = marisa_laser_init,
		.preload = marisa_laser_preload,
		.property = marisa_laser_property,
	},
};
