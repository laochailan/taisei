/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "common_tasks.h"
#include "random.h"
#include "util/glm.h"

void common_drop_items(cmplx pos, const ItemCounts *items) {
	for(int i = 0; i < ITEM_LAST - ITEM_FIRST; ++i) {
		for(int j = items->as_array[i]; j; --j) {
			spawn_item(pos, i + ITEM_FIRST);
			WAIT(2);
		}
	}
}

DEFINE_EXTERN_TASK(common_drop_items) {
	common_drop_items(*ARGS.pos, &ARGS.items);
}

void common_move_loop(cmplx *restrict pos, MoveParams *restrict mp) {
	for(;;) {
		move_update(pos, mp);
		YIELD;
	}
}

DEFINE_EXTERN_TASK(common_move) {
	if(ARGS.ent.ent) {
		TASK_BIND(ARGS.ent);
	}

	MoveParams p = ARGS.move_params;
	common_move_loop(ARGS.pos, &p);
}

DEFINE_EXTERN_TASK(common_move_ext) {
	if(ARGS.ent.ent) {
		TASK_BIND(ARGS.ent);
	}

	common_move_loop(ARGS.pos, ARGS.move_params);
}

DEFINE_EXTERN_TASK(common_call_func) {
	ARGS.func();
}

DEFINE_EXTERN_TASK(common_start_bgm) {
	stage_start_bgm(ARGS.bgm);
}

static Projectile *spawn_charge_particle_smoke(cmplx target, real power) {
	real scale = power * rng_range(0.7, 1);
	real opacity = power * rng_range(0.5, 0.8);
	cmplx pos = target + rng_dir() * 16 * power;
	MoveParams move = move_asymptotic_simple(0.5 * rng_dir(), 3);
	real angle = rng_angle();
	int timeout = rng_irange(30, 60);

	return PARTICLE(
		.sprite = "smoke",
		.pos = pos,
		.draw_rule = pdraw_timeout_scalefade_exp(0.01, scale, opacity, 0, 2),
		.move = move,
		.timeout = timeout,
		.flags = PFLAG_NOREFLECT | PFLAG_MANUALANGLE,
		.angle = angle,
		.layer = LAYER_PARTICLE_HIGH,
	);
}

static Projectile *spawn_charge_particle(cmplx target, real dist, const Color *clr, real power) {
	cmplx pos = target + rng_dir() * dist;
	MoveParams move = move_towards(target, rng_range(0.1, 0.2) + 0.05 * power);
	move.retention = 0.25 * cdir(1.5 * rng_sign());

	spawn_charge_particle_smoke(target, power);

	return PARTICLE(
		.sprite = "graze",
		.color = clr,
		.pos = pos,
		.draw_rule = pdraw_timeout_scalefade(2, 0.05, 0, 1),
		.move = move,
		.timeout = rng_irange(25, 35),
		.flags = PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE,
		.layer = LAYER_PARTICLE_HIGH,
	);
}

static void randomize_hue(Color *clr, float r) {
	float h, s, l, a = clr->a;
	color_get_hsl(clr, &h, &s, &l);
	h += rng_sreal() * r;
	*clr = *HSLA(h, s, l, a);
}

TASK(charge_sound_stopper, { SFXPlayID id; }) {
	stop_sound(ARGS.id);
}

static int common_charge_impl(
	int time,
	const cmplx *anchor,
	cmplx offset,
	const Color *color,
	const char *snd_charge,
	const char *snd_discharge
) {
	real dist = 256;
	int delay = 3;
	real rayfactor = 1.0 / time;
	float hue_rand = 0.02;
	SFXPlayID charge_snd_id = snd_charge ? play_sfx(snd_charge) : 0;
	DECLARE_ENT_ARRAY(Projectile, particles, 256);

	BoxedTask snd_stopper_task = { 0 };

	if(charge_snd_id) {
		snd_stopper_task = cotask_box(INVOKE_TASK_AFTER(&TASK_EVENTS(THIS_TASK)->finished, charge_sound_stopper, charge_snd_id));
	}

	int wait_time = 0;

	for(int i = 0; i < time; ++i, wait_time += WAIT(1)) {
		cmplx pos = *anchor + offset;

		ENT_ARRAY_COMPACT(&particles);
		ENT_ARRAY_FOREACH(&particles, Projectile *p, {
			p->move.attraction_point = pos;
		});

		if(i % delay) {
			continue;
		}

		int stage = (5 * i) / time;
		int nparts = stage + 1;
		real power = (stage + 1) / 6.0;
		real sdist = dist * glm_ease_quad_out(power);

		for(int j = 0; j < nparts; ++j) {
			Color c = *color;
			randomize_hue(&c, hue_rand);
			color_lerp(&c, RGBA(1, 1, 1, 0), rng_real() * 0.2);
			Projectile *p = spawn_charge_particle(pos, sdist * (1 + 0.1 * rng_sreal()), &c, power);
			ENT_ARRAY_ADD(&particles, p);
			color_mul_scalar(&c, 0.2);
		}

		Color c = *color;
		randomize_hue(&c, hue_rand);
		color_lerp(&c, RGBA(1, 1, 1, 0), rng_real() * 0.2);
		color_mul_scalar(&c, 0.5);

		ENT_ARRAY_ADD(&particles, PARTICLE(
			.sprite = "blast_huge_rays",
			.color = &c,
			.pos = pos,
			.draw_rule = pdraw_timeout_scalefade(0, 1, 1, 0),
			.move = move_towards(pos, 0.1),
			.timeout = 30,
			.flags = PFLAG_NOREFLECT | PFLAG_MANUALANGLE,
			.scale = glm_ease_bounce_out(rayfactor * (i + 1)),
			.angle = rng_angle(),
			.layer = LAYER_PARTICLE_HIGH,
		));
	}

	CANCEL_TASK(snd_stopper_task);

	if(snd_discharge) {
		replace_sfx(charge_snd_id, snd_discharge);
	} else {
		stop_sound(charge_snd_id);
	}

	Color c = *color;
	randomize_hue(&c, hue_rand);
	color_mul_scalar(&c, 2.0);

	cmplx pos = *anchor + offset;

	PARTICLE(
		.sprite = "blast_huge_halo",
		.color = &c,
		.pos = pos,
		.draw_rule = pdraw_timeout_scalefade(0, 2, 1, 0),
		.timeout = 30,
		.flags = PFLAG_NOREFLECT,
		.angle = rng_angle(),
		.layer = LAYER_PARTICLE_HIGH,
	);

	ENT_ARRAY_FOREACH(&particles, Projectile *p, {
		if(!(p->flags & PFLAG_MANUALANGLE)) {
			p->move.attraction = 0;
			p->move.acceleration += cnormalize(p->pos - pos);
			p->move.retention = 0.8;
		}
	});

	assert(time == wait_time);
	return wait_time;
}

int common_charge(int time, const cmplx *anchor, cmplx offset, const Color *color) {
	return common_charge_impl(time, anchor, offset, color, COMMON_CHARGE_SOUND_CHARGE, COMMON_CHARGE_SOUND_DISCHARGE);
}

int common_charge_static(int time, cmplx pos, const Color *color) {
	cmplx anchor = 0;
	return common_charge_impl(time, &anchor, pos, color, COMMON_CHARGE_SOUND_CHARGE, COMMON_CHARGE_SOUND_DISCHARGE);
}

int common_charge_custom(
	int time,
	const cmplx *anchor,
	cmplx offset,
	const Color *color,
	const char *snd_charge,
	const char *snd_discharge
) {
	cmplx local_anchor = 0;
	anchor = anchor ? anchor : &local_anchor;
	return common_charge_impl(
		time,
		anchor,
		offset,
		color,
		snd_charge,
		snd_discharge
	);
}

static void common_move_turn(
	MoveParams *move,
	int delay,
	int duration,
	real angle
) {
	WAIT(delay);
	cmplx r = cdir(angle / duration);
	move->retention *= r;
	WAIT(duration);
	move->retention /= r;
}

DEFINE_EXTERN_TASK(common_charge) {
	Color local_color;
	const Color *p_color = ARGS.color_ref;
	if(!p_color) {
		local_color = *NOT_NULL(ARGS.color);
		p_color = &local_color;
	}

	if(ARGS.bind_to_entity.ent) {
		TASK_BIND(ARGS.bind_to_entity);
	}

	common_charge_custom(
		ARGS.time,
		ARGS.anchor,
		ARGS.pos,
		p_color,
		ARGS.sound.charge,
		ARGS.sound.discharge
	);
}

DEFINE_EXTERN_TASK(common_set_bitflags) {
	assume(ARGS.pflags != NULL);
	*ARGS.pflags = ((*ARGS.pflags & ARGS.mask) | ARGS.set);
}

DEFINE_EXTERN_TASK(common_kill_projectile) {
	kill_projectile(TASK_BIND(ARGS.proj));
}

DEFINE_EXTERN_TASK(common_kill_enemy) {
	enemy_kill(TASK_BIND(ARGS.enemy));
}

cmplx common_wander(cmplx origin, double dist, Rect bounds) {
	int attempts = 32;
	double angle;
	cmplx dest;
	cmplx dir;

	// assert(point_in_rect(origin, bounds));

	while(attempts--) {
		angle = rng_angle();
		dir = cdir(angle);
		dest = origin + dist * dir;

		if(point_in_rect(dest, bounds)) {
			return dest;
		}
	}

	log_warn("Clipping fallback  origin = %f%+fi  dist = %f  bounds.top_left = %f%+fi  bounds.bottom_right = %f%+fi",
		creal(origin), cimag(origin),
		dist,
		creal(bounds.top_left), cimag(bounds.top_left),
		creal(bounds.bottom_right), cimag(bounds.bottom_right)
	);

	// TODO: implement proper line-clipping here?

	double step = cabs(bounds.bottom_right - bounds.top_left) / 16;
	dir *= step;
	dest = origin;

	while(point_in_rect(dest + dir, bounds)) {
		dest += dir;
	}

	return dest;
}

DEFINE_EXTERN_TASK(common_easing_animate) {
	float from = *ARGS.value;
	float scale = ARGS.to - from;
	float ftime = ARGS.duration;

	for(int t = 1; t <= ARGS.duration; t++) {
		YIELD;
		*ARGS.value = from + scale * ARGS.ease(t / ftime);
    }
}

DEFINE_EXTERN_TASK(common_move_turn) {
	common_move_turn(ARGS.move_params, ARGS.turn_delay, ARGS.turn_duration, ARGS.turn_angle);
}
