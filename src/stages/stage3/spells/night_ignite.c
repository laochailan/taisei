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

TASK(slave, { BoxedBoss boss; real rot_speed; real rot_initial; }) {
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

	WAIT(300);

	for(;;WAIT(180)) {
		int cnt = 5, i;

		for(i = 0; i < cnt; ++i) {
			PROJECTILE(
				.proto = pp_ball,
				.pos = slave->pos,
				.color = RGBA(0.5, 1.0, 0.5, 0),
				.move = move_accelerated(0, 0.02 * cdir(i*M_TAU/cnt)),
			);

			if(global.diff > D_Hard) {
				PROJECTILE(
					.proto = pp_ball,
					.pos = slave->pos,
					.color = RGBA(1.0, 1.0, 0.5, 0),
					.move = move_accelerated(0, 0.01 * cdir(i*2*M_TAU/cnt)),
				);
			}
		}

		// FIXME: better sound
		play_sfx("shot_special1");
	}
}

TASK(warnlaser, { BoxedLaser base_laser; }) {
	Laser *b = NOT_NULL(ENT_UNBOX(ARGS.base_laser));

	real f = 6;
	Laser *l = TASK_BIND(create_laser(
		b->pos, 90, 120, RGBA(1, 1, 1, 0), b->prule, NULL,
		f*b->args[0], b->args[1], f*b->args[2], b->args[3]
	));

	laser_make_static(l);

	for(int t = 0;; ++t, YIELD) {
		if(t == 90) {
			play_sfx_ex("laser1", 30, false);
		}

		laser_charge(l, t, 90, 10);
		l->color = *color_lerp(RGBA(0.2, 0.2, 1, 0), RGBA(1, 0.2, 0.2, 0), t / l->deathtime);
	}
}

TASK(laserbullet, { ProjPrototype *proto; Color *color; BoxedLaser laser; cmplx pos; real time_offset; }) {
	Projectile *p = TASK_BIND(PROJECTILE(
		.proto = ARGS.proto,
		.color = ARGS.color,
		.pos = ARGS.pos,
		.move = move_linear(0),
	));

	int t = 0;
	for(Laser *l; (l = ENT_UNBOX(ARGS.laser)); ++t, YIELD) {
		p->move.velocity = l->prule(l, t + ARGS.time_offset) - p->pos;
	}
}

DEFINE_EXTERN_TASK(stage3_spell_night_ignite) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_towards(VIEWPORT_W/2 + VIEWPORT_H*I/3, 0.05);
	BEGIN_BOSS_ATTACK(&ARGS);

	int nslaves = 7;

	for(int i = 0; i < nslaves; ++i) {
		INVOKE_SUBTASK(slave, ENT_BOX(boss),  1/70.0,  i*M_TAU/nslaves);
		INVOKE_SUBTASK(slave, ENT_BOX(boss), -1/70.0, -i*M_TAU/nslaves);
	}

	real dt = 200;
	real lt = difficulty_value(25, 50, 75, 100);
	real amp = M_PI/5;
	real freq = 0.05;
	int laserbullets = difficulty_value(8, 12, 16, 20);

	int t = 0;

	for(int cycle = 0;; ++cycle, t += WAIT(180)) {
		for(int step = 0; step < 12; ++step, t += WAIT(10)) {
			real a = step * M_PI/2.5 + cycle + t;
			cmplx vel = 2 * cdir(a);

			Laser *l1 = create_lasercurve3c(
				boss->pos, lt, dt,
				RGBA(0.3, 0.3, 1.0, 0.0),
				las_sine_expanding, vel, amp, freq
			);
			INVOKE_TASK(warnlaser, ENT_BOX(l1));

			Laser *l2 = create_lasercurve3c(
				boss->pos, lt * 1.5, dt,
				RGBA(1.0, 0.3, 0.3, 0.0),
				las_sine_expanding, vel, amp, freq - 0.002 * imin(global.diff, D_Hard)
			);
			INVOKE_TASK(warnlaser, ENT_BOX(l2));

			Laser *l3 = create_lasercurve3c(
				boss->pos, lt, dt,
				RGBA(0.3, 0.3, 1.0, 0.0),
				las_sine_expanding, vel, amp, freq - 0.004 * imin(global.diff, D_Hard)
			);
			INVOKE_TASK(warnlaser, ENT_BOX(l3));

			for(int i = 0; i < laserbullets; ++i) {
				#define LASERBULLLET(_proto, _color, _laser) \
					INVOKE_TASK(laserbullet, \
						.proto = _proto, \
						.color = _color, \
						.laser = ENT_BOX(_laser), \
						.pos = boss->pos, \
						.time_offset = -i \
					)

				LASERBULLLET(pp_plainball, RGBA(0.3, 0.3, 1.0, 0), l1);
				LASERBULLLET(pp_bigball,   RGBA(1.0, 0.3, 0.3, 0), l2);
				LASERBULLLET(pp_plainball, RGBA(0.3, 0.3, 1.0, 0), l3);

				play_sfx("shot1");
			}

			play_sfx_ex("shot_special1", 1, false);
		}
	}

	STALL;
}
