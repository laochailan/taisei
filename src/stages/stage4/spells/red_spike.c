/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "spells.h"
#include "common_tasks.h"
#include "../kurumi.h"

#include "global.h"

TASK(kurumi_redspike_slave, { cmplx pos; int direction; }) {
	Enemy *e = TASK_BIND(create_enemy1c(ARGS.pos, 1, kurumi_slave_visual, NULL, 0));
	e->flags = EFLAGS_GHOST;

	INVOKE_TASK_AFTER(&TASK_EVENTS(THIS_TASK)->finished, common_kill_enemy, ENT_BOX(e));

	e->move.velocity = ARGS.direction;
	e->move.retention = cdir(0.01 * ARGS.direction);

	int lifetime = difficulty_value(350, 400, 450, 500);
	int step = difficulty_value(16, 14, 12, 10);

	for(int i = 0; i < lifetime/step; i++, WAIT(step)) {
		float r = cimag(e->pos)/VIEWPORT_H;

		for(int d = 1; d >= -1; d -= 2) {
			PROJECTILE(
				.proto = pp_wave,
				.pos = e->pos + 10.0 * I * ARGS.direction,
				.color = RGB(r,0,0),
				.move = move_linear(d * 1.5 * I * e->move.velocity)
			);
		}

		play_sfx("shot1");
	}
}


TASK(kurumi_redspike_spawn_slaves, { BoxedBoss boss; int interval; }) {
	Boss *b = TASK_BIND(ARGS.boss);

	for(int i = 0;; i++) {
		INVOKE_SUBTASK(kurumi_redspike_slave, b->pos, 1 - 2 * (i&1));
		WAIT(ARGS.interval);
	}
}

DEFINE_EXTERN_TASK(kurumi_dryfountain) {
	Boss *b = INIT_BOSS_ATTACK();
	BEGIN_BOSS_ATTACK();

	INVOKE_SUBTASK(kurumi_redspike_spawn_slaves, .boss = ENT_BOX(b), .interval = 60);

	int step = difficulty_value(100, 50, 50, 50);
	int count = difficulty_value(8, 16, 16, 16);
	real speed = 3 * difficulty_value(1.1, 1.2, 1.2, 1.2);

	for(;;) {
		for(int i = 0; i < count; i++) {
			cmplx vel = speed * cdir(M_TAU / count * i) * cnormalize(global.plr.pos-b->pos);
			PROJECTILE(
				.proto = pp_bigball,
				.pos = b->pos,
				.color = RGBA(1.0, 0.0, 0.0, 0.0),
				.move = move_asymptotic_simple(vel, 3)
			);
		}

		play_sfx("shot_special1");
		WAIT(step);
	}
}


TASK(kurumi_redspike_animate, { BoxedBoss boss; }) {
	Boss *b = TASK_BIND(ARGS.boss);
	WAIT(80);
	aniplayer_queue(&b->ani, "muda", 0);
	WAIT(420);
	aniplayer_queue(&b->ani, "main", 0);
}	
	

DEFINE_EXTERN_TASK(kurumi_redspike) {
	Boss *b = INIT_BOSS_ATTACK();
	BEGIN_BOSS_ATTACK();

	INVOKE_SUBTASK(kurumi_redspike_spawn_slaves, .boss = ENT_BOX(b), .interval = 60);
	for(;;) {
		INVOKE_SUBTASK(kurumi_redspike_animate, ENT_BOX(b));
		WAIT(80);

		for(int rep = 0; rep < 2; rep++) {

			int step = difficulty_value(4, 4, 4, 2);

			for(int i = 0; i < 200; i += WAIT(step)) {
				RNG_ARRAY(rand, 2);
				cmplx offset = 100 * vrng_real(rand[0]) * vrng_dir(rand[1]); 
				cmplx dir = cnormalize(global.plr.pos - b->pos - offset);

				PROJECTILE(
					.proto = pp_rice,
					.pos = b->pos+offset,
					.color = RGBA(1, 0, 0, 0),
					.move = move_accelerated(-dir, 0.05 * dir),
				);

				play_sfx_ex("warp",0,false);
			}


			WAIT(40);
		}
	}
}
