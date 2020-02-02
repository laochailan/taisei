/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "corotest.h"
#include "coroutine.h"
#include "global.h"

static CoSched *cotest_sched;

static void cotest_stub_proc(void) { }

TASK(test_enemy, { double hp; complex pos; complex dir; }) {
	Enemy *e = create_enemy1c(ARGS.pos, ARGS.hp, BigFairy, NULL, 0);
	int eref = add_ref(e);

	#define BREAK_CONDITION !UPDATE_REF(eref, e)

	BYIELD;

	while(true) {
		// wander around for a bit...
		for(int i = 0; i < 20; ++i) {
			e->pos += ARGS.dir;
			BYIELD;
		}

		// stop and...
		BWAIT(10);

		// pew pew!!!
		int pcount = 10 + 10 * frand();
		for(int i = 0; i < pcount; ++i) {
			complex aim = global.plr.pos - e->pos;
			aim *= 5 * cexp(I*M_PI*0.1*nfrand()) / cabs(aim);

			PROJECTILE(
				.pos = e->pos,
				.proto = pp_rice,
				.color = RGBA(1.0, 0.0, i / (pcount - 1.0), 0.0),
				.rule = linear,
				.args = { aim },
			);

			BWAIT(3);
		}

		// keep wandering, randomly
		ARGS.dir *= cexp(I*M_PI*nfrand());
		BYIELD;
	}

BREAK:
	log_debug("enemy died!");
	free_ref(eref);
}

TASK(stage_main, { int ignored; }) {
	YIELD;

	WAIT(30);
	log_debug("test 1! %i", global.timer);
	WAIT(60);
	log_debug("test 2! %i", global.timer);

	for(;;) {
		INVOKE_TASK(test_enemy, 9000, CMPLX(VIEWPORT_W, VIEWPORT_H) * 0.5, 3*I);
		WAIT(240);
	}
}

static void cotest_begin(void) {
	cotest_sched = cosched_new();
	cosched_set_invoke_target(cotest_sched);
	INVOKE_TASK(stage_main, 0);
}

static void cotest_end(void) {
	cosched_free(cotest_sched);
}

static void cotest_events(void) {
	if(!global.gameover && !cosched_run_tasks(cotest_sched)) {
		log_debug("over!");
		stage_finish(GAMEOVER_SCORESCREEN);
	}
}

StageProcs corotest_procs = {
	.begin = cotest_begin,
	.preload = cotest_stub_proc,
	.end = cotest_end,
	.draw = cotest_stub_proc,
	.update = cotest_stub_proc,
	.event = cotest_events,
	.shader_rules = (ShaderRule[]) { NULL },
};
