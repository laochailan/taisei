/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"
#include "../wriggle.h"

#include "common_tasks.h"
#include "global.h"

TASK(cancel_event, { CoEvent *event; }) {
	coevent_cancel(ARGS.event);
}

TASK(laser_bullet, { BoxedProjectile p; BoxedLaser l; CoEvent *event; int event_time; }) {
	Laser *l = NOT_NULL(ENT_UNBOX(ARGS.l));

	if(ARGS.event) {
		INVOKE_TASK_AFTER(&TASK_EVENTS(THIS_TASK)->finished, cancel_event, ARGS.event);
	}

	Projectile *p = TASK_BIND(ARGS.p);

	for(int t = 0; (l = ENT_UNBOX(ARGS.l)); ++t, YIELD) {
		p->pos = l->prule(l, t);

		if(t == 0) {
			p->prevpos = p->pos;
		}

		if(t == ARGS.event_time && ARGS.event) {
			coevent_signal(ARGS.event);
		}
	}

	kill_projectile(p);
}

TASK(rocket, { BoxedBoss boss; cmplx pos; cmplx dir; Color color; real phase; real accel_rate; }) {
	Boss *boss = TASK_BIND(ARGS.boss);

	real dt = 60;
	Laser *l = create_lasercurve4c(
		ARGS.pos, dt, dt, &ARGS.color, las_sine_expanding, 2.5*ARGS.dir, M_PI/20, 0.2, ARGS.phase
	);

	Projectile *p = PROJECTILE(
		.proto = pp_ball,
		.color = RGB(1.0, 0.4, 0.6),
		.flags = PFLAG_NOMOVE,
	);

	COEVENTS_ARRAY(phase2, explosion) events;
	TASK_HOST_EVENTS(events);

	INVOKE_TASK(laser_bullet, ENT_BOX(p), ENT_BOX(l), &events.phase2, dt);
	WAIT_EVENT_OR_DIE(&events.phase2);
	play_sfx("redirect");
	// if we get here, p must be still alive and valid

	cmplx dist = global.plr.pos - p->pos;
	cmplx accel = ARGS.accel_rate * cnormalize(dist);
	dt = sqrt(2 * cabs(dist) / ARGS.accel_rate);
	l = create_lasercurve2c(p->pos, dt, dt, RGBA(0.4, 0.9, 1.0, 0.0), las_accel, 0, accel);
	l->width = 15;
	INVOKE_TASK(laser_bullet, ENT_BOX(p), ENT_BOX(l), &events.explosion, dt);
	WAIT_EVENT_OR_DIE(&events.explosion);
	// if we get here, p must be still alive and valid

	int cnt = 22;
	real rot = (global.frames - NOT_NULL(boss->current)->starttime) * 0.0037 * global.diff;
	Color *c = HSLA(fmod(rot, M_TAU) / (M_TAU), 1.0, 0.5, 0);
	real boost = difficulty_value(4, 6, 8, 10);

	for(int i = 0; i < cnt; ++i) {
		real f = (real)i/cnt;

		cmplx dir = cdir(M_TAU * f + rot);
		cmplx v = (1.0 + psin(M_TAU * 9 * f)) * dir;

		PROJECTILE(
			.proto = pp_thickrice,
			.pos = p->pos,
			.color = c,
			.move = move_asymptotic_simple(v, boost),
		);
	}

	real to = rng_range(30, 35);
	real scale = rng_range(2, 2.5);

	PARTICLE(
		.proto = pp_blast,
		.pos = p->pos,
		.color = c,
		.timeout = to,
		.draw_rule = pdraw_timeout_scalefade(0.01, scale, 1, 0),
		.angle = rng_angle(),
	);

	// FIXME: better sound
	play_sfx("enemydeath");
	play_sfx("shot1");
	play_sfx("shot1_special");

	kill_projectile(p);
}

TASK(rocket_slave, { BoxedBoss boss; real rot_speed; real rot_initial; }) {
	Boss *boss = TASK_BIND(ARGS.boss);

	cmplx dir;

	WriggleSlave *slave = stage3_host_wriggle_slave(boss->pos);
	INVOKE_SUBTASK(wriggle_slave_damage_trail, ENT_BOX(slave));
	INVOKE_SUBTASK(wriggle_slave_follow,
		.slave = ENT_BOX(slave),
		.boss = ENT_BOX(boss),
		.rot_speed = ARGS.rot_speed,
		.rot_initial = ARGS.rot_initial,
		.out_dir = &dir
	);

	int rperiod = difficulty_value(220, 200, 180, 160);
	real laccel = difficulty_value(0.15, 0.2, 0.25, 0.3);

	WAIT(rperiod/2);

	for(;;WAIT(rperiod)) {
		play_sfx("laser1");
		INVOKE_TASK(rocket, ENT_BOX(boss), slave->pos, dir, *RGBA(1.0, 1.0, 0.5, 0.0), 0, laccel);
		INVOKE_TASK(rocket, ENT_BOX(boss), slave->pos, dir, *RGBA(0.5, 1.0, 0.5, 0.0), M_PI, laccel);
	}
}

DEFINE_EXTERN_TASK(stage3_spell_moonlight_rocket) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_towards(VIEWPORT_W/2 + VIEWPORT_H*I/2.5, 0.05);
	BEGIN_BOSS_ATTACK(&ARGS);

	int nslaves = difficulty_value(2, 3, 4, 5);

	for(int i = 0; i < nslaves; ++i) {
		for(int s = -1; s < 2; s += 2) {
			INVOKE_SUBTASK(rocket_slave, ENT_BOX(boss), s/70.0, s*i*M_TAU/nslaves);
		}
	}

	// keep subtasks alive
	STALL;
}
