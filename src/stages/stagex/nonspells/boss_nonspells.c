/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "boss.h"
#include "taisei.h"

#include "nonspells.h"
#include "../yumemi.h"

#include "stage.h"
#include "common_tasks.h"
#include "global.h"

TASK(wallproj, { BoxedProjectile p; }) {
	Projectile *p = TASK_BIND(ARGS.p);

	for(;;YIELD) {
		real x = creal(p->pos);
		real y = cimag(p->pos);
		real th = 16 * rng_sreal();

		if(x < th || x > VIEWPORT_W - th || y < th || y > VIEWPORT_H - th) {
			break;
		}
	}

	p->move.acceleration = 0;
	p->move.retention = 1;
	p->move.velocity = 0;

	projectile_set_prototype(p, pp_ball);
	p->color.a = 0;
	spawn_projectile_highlight_effect(p);

	real rate = 0;

	for(;;YIELD) {
		p->color.g = rate;
		p->move.acceleration = rate * 0.05 * cnormalize(global.plr.pos - p->pos);
// 		p->move.acceleration *= 0.6;
// 		p->move.acceleration += rate * 0.05 * cnormalize(global.plr.pos - p->pos);
		rate = approach(rate, 1, 0.0002);
	}
}

static void yumemi_slave_aimed_burst(YumemiSlave *slave, int count, real *a, real s, bool wallproj) {
	for(int burst = 0; burst < count; ++burst) {
		int cnt = 3;

		for(int i = 0; i < cnt; ++i) {
			play_sfx_loop("shot1_loop");

			cmplx pos = slave->pos + 32 * cdir(s * (*a + i*M_TAU/cnt));
			cmplx aim = cnormalize(global.plr.pos - pos);
			aim *= 10;

			Projectile *p = PROJECTILE(
				.proto = pp_rice,
				.color = RGBA(1, 0, 0.25, 0),
				.pos = pos,
				.move = move_asymptotic_simple(aim, -2),
			);

			if(wallproj) {
				INVOKE_TASK(wallproj, ENT_BOX(p));
			}

			YIELD;
		}

		*a += M_PI/32;
	}

	WAIT(20);
}

static void yumemi_slave_aimed_lasers(YumemiSlave *slave, real s, cmplx target) {
	int cnt = 32;
	int t = 2;

	play_sfx_delayed("laser1", 3, 1, 50);
	real g = carg(target - slave->pos);
	real angle_ofs = (s < 0) * M_PI;

	for(int i = 0; i < cnt; ++i) {
		real x = i/(real)cnt;
		cmplx o = 32 * cdir(s * x * M_TAU + g + M_PI/2 + angle_ofs);
		cmplx pos = slave->pos + o;
		cmplx aim = cnormalize(target - pos + o);
		Color *c = RGBA(0.1 + 0.9 * x * x, 1 - 0.9 * (1 - pow(1 - x, 2)), 0.1, 0);
		create_laserline(pos, 40 * aim, 60 + i * t, 80 + i * t, c);
	}

	WAIT(10 + t * cnt);
}

static void yumemi_slave_aimed_funnel(YumemiSlave *slave, int count, real *a, real s, bool wallproj) {
	real max_angle = M_PI/2;
	real min_angle = M_PI/24;

	for(int burst = 0; burst < count; ++burst) {
		real bf = burst / (count - 1.0);
		cmplx rot = cdir(lerp(max_angle, min_angle, bf) * 0.03);

		int cnt = 3;
		for(int i = 0; i < cnt; ++i) {
			play_sfx_loop("shot1_loop");

			cmplx pos = slave->pos + 32 * cdir(s * (*a + i*M_TAU/cnt));
			cmplx aim = cnormalize(global.plr.pos - pos);
			aim *= 10;

			Projectile *p;

			p = PROJECTILE(
				.proto = pp_crystal,
				.color = RGBA(1, 0.4, 1 - bf, 0),
				.pos = pos,
				// .move = move_asymptotic_simple(aim * rot, -2),
				.move = { .velocity = aim, .retention = rot, },
			);

			if(wallproj) {
				INVOKE_TASK(wallproj, ENT_BOX(p));
			}

			p = PROJECTILE(
				.proto = pp_crystal,
				.color = RGBA(1, 0.4, 1 - bf, 0),
				.pos = pos,
				// .move = move_asymptotic_simple(aim / rot, -2),
				.move = { .velocity = aim, .retention = 1/rot, },
			);

			if(wallproj) {
				INVOKE_TASK(wallproj, ENT_BOX(p));
			}

			YIELD;
		}

		*a += M_PI/32;
	}

	WAIT(20);
}

TASK(yumemi_opening_slave, {
	cmplx pos;
	int type;
	CoEvent *sync_event;
}) {
	YumemiSlave *slave = stagex_host_yumemi_slave(ARGS.pos, ARGS.type);
	WAIT_EVENT_ONCE(ARGS.sync_event);

	WAIT(ARGS.type * 60);

	real s = 1 - 2 * ARGS.type;
	real a = 0;

	for(;;) {
		yumemi_slave_aimed_burst(slave, 25, &a, s, false);
		yumemi_slave_aimed_lasers(slave, s, global.plr.pos);
		WAIT(40);
	}
}

DEFINE_EXTERN_TASK(stagex_boss_nonspell_1) {
	STAGE_BOOKMARK(non1);

	Boss *boss = INIT_BOSS_ATTACK(&ARGS);

	CoEvent *sync_event = &ARGS.attack->events.started;

	cmplx p = VIEWPORT_W/2 + 100*I;
	real xofs = 140;

	INVOKE_SUBTASK_DELAYED(20, yumemi_opening_slave,
		.pos = p - xofs,
		.type = 0,
		.sync_event = sync_event
	);

	INVOKE_SUBTASK_DELAYED(40, yumemi_opening_slave,
		.pos = p + xofs,
		.type = 1,
		.sync_event = sync_event
	);

	WAIT_EVENT_ONCE(&ARGS.attack->events.initiated);
	boss->move = move_towards(VIEWPORT_W/2 + 180*I, 0.015);
	BEGIN_BOSS_ATTACK(&ARGS);
	AWAIT_SUBTASKS;
}

TASK(yumemi_non2_slave, {
	cmplx pos;
	int type;
	CoEvent *sync_event;
}) {
	YumemiSlave *slave = stagex_host_yumemi_slave(ARGS.pos, ARGS.type);
	WAIT_EVENT_ONCE(ARGS.sync_event);

	WAIT(ARGS.type * 60);

	real s = 1 - 2 * ARGS.type;
	real a = 0;

	for(;;) {
		yumemi_slave_aimed_funnel(slave, 20, &a, s, false);
		WAIT(10);
		yumemi_slave_aimed_lasers(slave, s, global.plr.pos);
		WAIT(40);
		// yumemi_slave_aimed_burst(slave, 15, &a, s);
	}
}

DEFINE_EXTERN_TASK(stagex_boss_nonspell_2) {
	STAGE_BOOKMARK(non2);

	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	CoEvent *sync_event = &ARGS.attack->events.started;

	cmplx p = VIEWPORT_W/2 + 100*I;
	real xofs = 140;

	INVOKE_SUBTASK_DELAYED(20, yumemi_non2_slave,
		.pos = p - xofs,
		.type = 0,
		.sync_event = sync_event
	);

	INVOKE_SUBTASK_DELAYED(45, yumemi_non2_slave,
		.pos = p + xofs,
		.type = 1,
		.sync_event = sync_event
	);

	boss->move = move_towards(VIEWPORT_W/2 + 180*I, 0.015);

	BEGIN_BOSS_ATTACK(&ARGS);
	AWAIT_SUBTASKS;
}

TASK(yumemi_non3_slave, {
	cmplx pos;
	int type;
	CoEvent *sync_event;
}) {
	YumemiSlave *slave = stagex_host_yumemi_slave(ARGS.pos, ARGS.type);
	WAIT_EVENT_ONCE(ARGS.sync_event);

	WAIT(ARGS.type * 60);

	real s = 1 - 2 * ARGS.type;
	real a = 0;

	for(;;) {
		yumemi_slave_aimed_funnel(slave, 20, &a, s, false);
		WAIT(10);
		yumemi_slave_aimed_lasers(slave, s, global.plr.pos);
		WAIT(40);
		yumemi_slave_aimed_burst(slave, 15, &a, s, false);
	}
}

DEFINE_EXTERN_TASK(stagex_boss_nonspell_3) {
	STAGE_BOOKMARK(non3);

	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	CoEvent *sync_event = &ARGS.attack->events.started;

	cmplx p = VIEWPORT_W/2 + 100*I;
	real xofs = 140;

	INVOKE_SUBTASK_DELAYED(20, yumemi_non3_slave,
		.pos = p - xofs,
		.type = 0,
		.sync_event = sync_event
	);

	INVOKE_SUBTASK_DELAYED(45, yumemi_non3_slave,
		.pos = p + xofs,
		.type = 1,
		.sync_event = sync_event
	);

	boss->move = move_towards(VIEWPORT_W/2 + 180*I, 0.015);

	BEGIN_BOSS_ATTACK(&ARGS);
	AWAIT_SUBTASKS;
}

TASK(yumemi_non4_slave, {
	cmplx pos;
	int type;
	CoEvent *sync_event;
}) {
	YumemiSlave *slave = stagex_host_yumemi_slave(ARGS.pos, ARGS.type);
	WAIT_EVENT_ONCE(ARGS.sync_event);

	WAIT(ARGS.type * 60);

	real s = 1 - 2 * ARGS.type;
	real a = 0;

	for(;;) {
		yumemi_slave_aimed_burst(slave, 15, &a, s, true);
		WAIT(60);
	}
}

DEFINE_EXTERN_TASK(stagex_boss_nonspell_4) {
	STAGE_BOOKMARK(non4);

	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	CoEvent *sync_event = &ARGS.attack->events.started;

	cmplx p = VIEWPORT_W/2 + 100*I;
	real xofs = 140;

	INVOKE_SUBTASK_DELAYED(20, yumemi_non4_slave,
		.pos = p - xofs,
		.type = 0,
		.sync_event = sync_event
	);

	INVOKE_SUBTASK_DELAYED(45, yumemi_non4_slave,
		.pos = p + xofs,
		.type = 1,
		.sync_event = sync_event
	);

	boss->move = move_towards(VIEWPORT_W/2 + 180*I, 0.015);

	BEGIN_BOSS_ATTACK(&ARGS);
	AWAIT_SUBTASKS;
}

TASK(yumemi_non5_slave, {
	cmplx pos;
	int type;
	CoEvent *sync_event;
}) {
	YumemiSlave *slave = stagex_host_yumemi_slave(ARGS.pos, ARGS.type);
	WAIT_EVENT_ONCE(ARGS.sync_event);

	WAIT(ARGS.type * 60);

	real s = 1 - 2 * ARGS.type;
	real a = 0;

	for(int i = 0; i < 2 * ARGS.type; ++i) {
		yumemi_slave_aimed_burst(slave, 15, &a, s, true);
		WAIT(60);
	}

	for(;;) {
		int charge_time = 60;

		INVOKE_SUBTASK(common_charge,
			.pos = slave->pos,
			.color = RGBA(0.3, 0.3, 0.8, 0),
			.time = charge_time,
			.sound = COMMON_CHARGE_SOUNDS
		);

		WAIT(charge_time);

		yumemi_slave_aimed_funnel(slave, 10, &a, s, true);
		WAIT(60);

		for(int i = 0; i < 5; ++i) {
			yumemi_slave_aimed_burst(slave, 15, &a, s, true);
			WAIT(60);
		}
	}
}

DEFINE_EXTERN_TASK(stagex_boss_nonspell_5) {
	STAGE_BOOKMARK(non5);

	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	CoEvent *sync_event = &ARGS.attack->events.started;

	cmplx p = VIEWPORT_W/2 + 100*I;
	real xofs = 140;

	INVOKE_SUBTASK_DELAYED(20, yumemi_non5_slave,
		.pos = p - xofs,
		.type = 0,
		.sync_event = sync_event
	);

	INVOKE_SUBTASK_DELAYED(45, yumemi_non5_slave,
		.pos = p + xofs,
		.type = 1,
		.sync_event = sync_event
	);

	boss->move = move_towards(VIEWPORT_W/2 + 180*I, 0.015);

	BEGIN_BOSS_ATTACK(&ARGS);
	AWAIT_SUBTASKS;
}

TASK(yumemi_non6_slave, {
	cmplx pos;
	cmplx center;
	int type;
}) {
	YumemiSlave *slave = stagex_host_yumemi_slave(ARGS.pos, ARGS.type);

	WAIT(120);

	real a = 0;
	real s = 1 - ARGS.type * 2;

// 	yumemi_slave_aimed_lasers(slave, 1, global.plr.pos);
// 	yumemi_slave_aimed_funnel(slave, 10, &a, s, false);

	cmplx r = cdir(0);

	for(;;) {
		int cnt = 12;

		for(int i = 0; i < cnt; ++i) {
			cmplx d = cdir(s * (a + i*M_TAU/cnt));
			cmplx pos = slave->pos + 32 * cdir(s * (a + i*M_TAU/cnt));
			cmplx aim = 2.5 * -d;

			Projectile *p;

			p = PROJECTILE(
				.proto = pp_crystal,
				.color = RGBA(0.5, 0, 1, 1),
				.pos = pos,
				.move = move_linear(aim * r),
				.layer = LAYER_BULLET | 0x10,
			);

			p = PROJECTILE(
				.proto = pp_crystal,
				.color = RGBA(0.5, 0, 1, 1),
				.pos = pos,
				.move = move_linear(aim / r),
				.layer = LAYER_BULLET | 0x10,
			);

			//YIELD;
			WAIT(1);
		}

 		a += M_PI/32;
	}

	STALL;
}

DEFINE_EXTERN_TASK(stagex_boss_nonspell_6) {
	STAGE_BOOKMARK(non6);

#if 0
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_towards(VIEWPORT_W/2 + 180*I, 0.015);
	BEGIN_BOSS_ATTACK(&ARGS);

	cmplx c = (VIEWPORT_W + VIEWPORT_H*I) * 0.5;
	real dist = 180;
	int cnt = 6;

	for(int i = 0; i < cnt; ++ i) {
		INVOKE_SUBTASK(yumemi_non6_slave,
			.pos = c + dist * cdir(i*M_TAU/cnt + M_PI/2),
			.type = i & 1,
			.center = c
		);
		WAIT(25);
	}

	STALL;
#endif

	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	CoEvent *sync_event = &ARGS.attack->events.started;

	cmplx p = VIEWPORT_W/2 + 100*I;
	real xofs = 140;

	INVOKE_SUBTASK_DELAYED(20, yumemi_non3_slave,
		.pos = p - xofs,
		.type = 0,
		.sync_event = sync_event
	);

	INVOKE_SUBTASK_DELAYED(45, yumemi_non3_slave,
		.pos = p + xofs,
		.type = 1,
		.sync_event = sync_event
	);

	INVOKE_SUBTASK_DELAYED(90, yumemi_non6_slave,
		.pos = p - xofs / 2 + 32 * I,
		.type = 1
	);

	INVOKE_SUBTASK_DELAYED(115, yumemi_non6_slave,
		.pos = p + xofs / 2 + 32 * I,
		.type = 0
	);

	boss->move = move_towards(VIEWPORT_W/2 + 180*I, 0.015);

	BEGIN_BOSS_ATTACK(&ARGS);
	AWAIT_SUBTASKS;
}
