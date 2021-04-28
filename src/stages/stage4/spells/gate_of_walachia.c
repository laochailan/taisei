/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "common_tasks.h".h"
#include "spells.h"
#include "../kurumi.h"

#include "global.h"

TASK(kurumi_walachia_slave_move_turn, { cmplx *vel; int duration; cmplx offset; }) {
	for(int i = 0; i < ARGS.duration; i++, YIELD) {
		(*ARGS.vel) -= 0.02*ARGS.offset;
		(*ARGS.vel) *= cdir(0.02);
	}
}

TASK(kurumi_walachia_slave_move, { BoxedEnemy slave; cmplx direction; }) {
	Enemy *e = TASK_BIND(ARGS.slave);

	cmplx vel = ARGS.direction;
	INVOKE_SUBTASK_DELAYED(40, kurumi_walachia_slave_move_turn, &vel, 60, ARGS.direction);
	
	for(int i = 0;; i++, YIELD) {
		e->move = move_linear(vel * (1 + psin(i / 10.0)));
	}

}	

TASK(kurumi_walachia_slave, { cmplx pos; cmplx direction; int lifetime; }) {
	Enemy *e = TASK_BIND(create_enemy3c(ARGS.pos, 1, kurumi_slave_visual, NULL,  0, 0, 0));
	e->flags = EFLAGS_GHOST;

	INVOKE_SUBTASK(kurumi_walachia_slave_move, ENT_BOX(e), ARGS.direction);
	
	
	int step = difficulty_value(16, 14, 12, 10);
	for(int i = 0; i < ARGS.lifetime; i += WAIT(step)) {
		float r = cimag(e->pos)/VIEWPORT_H;

		for(int j = 1; j >= -1; j -= 2) {
			PROJECTILE(
				.proto = pp_wave,
				.pos = e->pos + j * 10.0 * I * ARGS.direction,
				.color = RGB(r,0,0),
				.move = move_accelerated(j * 2.0 * I * ARGS.direction, -0.01*cnormalize(e->move.velocity))
			);
		}

		play_sfx("shot1");
	}

	enemy_kill(e);
}

DEFINE_EXTERN_TASK(kurumi_walachia) {
	Boss *b = INIT_BOSS_ATTACK();
	BEGIN_BOSS_ATTACK();

	int slave_count = difficulty_value(5, 7, 9, 11);
	int duration = 400;


	for(int run = 0;; run++) {
		common_charge(40, &b->pos, 0, RGBA(1.0, 0, 0, 0));
		for(int i = 0; i < slave_count; i++) {
			INVOKE_SUBTASK(kurumi_walachia_slave,
				.pos = b->pos,
				.direction = cdir(M_TAU / slave_count * i + 0.2 * run),
				.lifetime = duration + 200);
		}
		WAIT(duration);
	}
}
