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

typedef struct {
	cmplx pos;
	MoveParams move;
} Mover; // make this global?

TASK(mover_update, { Mover *m; }) {
	for(;;) {
		move_update(&ARGS.m->pos, &ARGS.m->move);
		YIELD;
	}
}


TASK(kurumi_aniwall_bullet, { cmplx pos; MoveParams move; }) {
	Projectile *p = TASK_BIND(PROJECTILE(
		.proto = pp_ball,
		.pos = ARGS.pos,
		.color = RGB(1,0,0),
		.move = ARGS.move,
	));

	for(;;) {
		p->color.r = cimag(p->pos)/VIEWPORT_H;
		YIELD;
	}
}

TASK(kurumi_aniwall_slave_move, { Mover *m; cmplx direction; }) {

	cmplx corners[] = {
		0,
		VIEWPORT_W,
		VIEWPORT_W + I * VIEWPORT_H,
		I * VIEWPORT_H,
	};

	real speed = 10;

	int order;
	int next;
	if(creal(ARGS.direction) > 0) {
		next = 2;
		order = 1;
	} else {
		next = 3;
		order = -1;
	}

	for(;; next = (next + order + ARRAY_SIZE(corners)) % ARRAY_SIZE(corners)) {
		cmplx d = corners[next] - ARGS.m->pos;
		ARGS.m->move = move_linear(speed * cnormalize(d));

		int travel_time = cabs(d) / speed;
		WAIT(travel_time);
	}
}


TASK(kurumi_aniwall_slave, { cmplx pos; cmplx direction; }) {
	Mover m = {
		.pos = ARGS.pos,
		.move = move_accelerated(0, 0.2*ARGS.direction)
	};
	INVOKE_SUBTASK(mover_update, &m);
	INVOKE_SUBTASK(stage4_boss_slave_visual, &m.pos, .interval = 1);

	create_lasercurve2c(ARGS.pos, 50, 80, RGBA(1.0, 0.4, 0.4, 0.0), las_accel, 0, m.move.acceleration);

	real target_edge = creal(ARGS.direction) > 0 ? VIEWPORT_W : 0;
	int impact_time = sqrt(2 * fabs(creal(ARGS.pos - target_edge) / creal(m.move.acceleration)));

	WAIT(impact_time);
	
	m.move = move_linear(10 * I * sign(cimag(ARGS.direction)));

	INVOKE_SUBTASK(kurumi_aniwall_slave_move, &m, ARGS.direction);

	int step = difficulty_value(6, 5, 2, 1);
	for(int i = 0;; i++, WAIT(step)) {
		cmplx vel = sign(creal(ARGS.direction)) * I * cnormalize(m.move.velocity);
		if(cimag(vel) > -0.1 || global.diff > D_Easy) {
			play_sfx("shot1");
			INVOKE_TASK(kurumi_aniwall_bullet,
				.pos = m.pos + I * vel * 20 * rng_sreal(),
				.move = move_linear(vel)
			);
		}
	}
}

DEFINE_EXTERN_TASK(kurumi_aniwall) {
	Boss *b = INIT_BOSS_ATTACK();
	BEGIN_BOSS_ATTACK();

	b->move = move_towards((VIEWPORT_W + I * VIEWPORT_H) * 0.5, 0.0005);
	b->move.retention = cdir(0.01);


	aniplayer_queue(&b->ani, "muda", 0);
	play_sfx("laser1");
	INVOKE_SUBTASK(kurumi_aniwall_slave, .pos = b->pos, .direction = cdir(0.4));
	INVOKE_SUBTASK(kurumi_aniwall_slave, .pos = b->pos, .direction = cdir(M_PI - 0.4));
	STALL;
}
