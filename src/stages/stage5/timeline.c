/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "timeline.h"
#include "stage5.h"
#include "nonspells/nonspells.h"
#include "spells/spells.h"

#include "coroutine.h"
#include "enemy_classes.h"

MODERNIZE_THIS_FILE_AND_REMOVE_ME

TASK(boss_appear_stub, NO_ARGS) {
	log_warn("FIXME");
}

static void stage5_dialog_pre_boss(void) {
	PlayerMode *pm = global.plr.mode;
	Stage5PreBossDialogEvents *e;
	INVOKE_TASK_INDIRECT(Stage5PreBossDialog, pm->dialog->Stage5PreBoss, &e);
	INVOKE_TASK_WHEN(&e->boss_appears, boss_appear_stub);
	INVOKE_TASK_WHEN(&e->music_changes, common_start_bgm, "stage5boss");
}

static void stage5_dialog_post_boss(void) {
	PlayerMode *pm = global.plr.mode;
	INVOKE_TASK_INDIRECT(Stage5PostBossDialog, pm->dialog->Stage5PostBoss);
}

static void stage5_dialog_post_midboss(void) {
	PlayerMode *pm = global.plr.mode;
	INVOKE_TASK_INDIRECT(Stage5PostMidBossDialog, pm->dialog->Stage5PostMidBoss);
}

static int stage5_laserfairy(Enemy *e, int t) {
	TIMER(&t)
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 5, ITEM_POWER, 5);
		return 1;
	}

	FROM_TO(0, 100, 1) {
		GO_TO(e, e->pos0 + e->args[0] * 100, 0.05);
	}

	if(t > 700)
		e->pos -= e->args[0];

	FROM_TO(100, 700, (7-global.diff)*(1+(int)creal(e->args[1]))) {
		cmplx n = cexp(I*carg(global.plr.pos-e->pos)+(0.2-0.02*global.diff)*I*_i);
		float fac = (0.5+0.2*global.diff);
		create_lasercurve2c(e->pos, 100, 300, RGBA(0.7, 0.3, 1, 0), las_accel, fac*4*n, fac*0.05*n);
		PROJECTILE(
			.proto = pp_plainball,
			.pos = e->pos,
			.color = RGBA(0.7, 0.3, 1, 0),
			.rule = accelerated,
			.args = { fac*4*n, fac*0.05*n }
		);
		play_sfx_ex("shot_special1", 0, true);
	}

	return 1;
}

static void iku_intro(Boss *b, int t) {
	GO_TO(b, VIEWPORT_W/2+240.0*I, 0.015);

	if(t == 160)
		stage5_dialog_pre_boss();
}

Boss* stage5_spawn_iku(cmplx pos) {
	Boss *b = create_boss("Nagae Iku", "iku", pos);
	boss_set_portrait(b, "iku", NULL, "normal");
	b->glowcolor = *RGBA_MUL_ALPHA(0.2, 0.4, 0.5, 0.5);
	b->shadowcolor = *RGBA_MUL_ALPHA(0.65, 0.2, 0.75, 0.5);
	return b;
}

static void midboss_dummy(Boss *b, int t) { }

static Boss *stage5_spawn_midboss(void) {
	Boss *b = create_boss("Bombs?", "iku_mid", VIEWPORT_W+800.0*I);
	b->glowcolor = *RGB(0.2, 0.4, 0.5);
	b->shadowcolor = *RGBA_MUL_ALPHA(0.65, 0.2, 0.75, 0.5);

	Attack *a = boss_add_attack(b, AT_SurvivalSpell, "Discharge Bombs", 16, 10, iku_mid_intro, NULL);
	boss_set_attack_bonus(a, 5);

	// suppress the boss death effects (this triggers the "boss fleeing" case)
	boss_add_attack(b, AT_Move, "", 0, 0, midboss_dummy, NULL);

	boss_engage(b);
	b->attacks->starttime = global.frames;	// HACK: thwart attack delay

	return b;
}

static Boss* stage5_spawn_boss(void) {
	Boss *b = stage5_spawn_iku(VIEWPORT_W/2-200.0*I);

	boss_add_attack(b, AT_Move, "Introduction", 4, 0, iku_intro, NULL);
	boss_add_attack(b, AT_Normal, "Bolts1", 40, 24000, iku_bolts, NULL);
	boss_add_attack_from_info(b, &stage5_spells.boss.atmospheric_discharge, false);
	boss_add_attack(b, AT_Normal, "Bolts2", 45, 27000, iku_bolts2, NULL);
	boss_add_attack_from_info(b, &stage5_spells.boss.artificial_lightning, false);
	boss_add_attack(b, AT_Normal, "Bolts3", 50, 30000, iku_bolts3, NULL);

	if(global.diff < D_Hard) {
		boss_add_attack_from_info(b, &stage5_spells.boss.induction_field, false);
	} else {
		boss_add_attack_from_info(b, &stage5_spells.boss.inductive_resonance, false);
	}
	boss_add_attack_from_info(b, &stage5_spells.boss.natural_cathode, false);

	boss_add_attack_from_info(b, &stage5_spells.extra.overload, false);

	boss_engage(b);
	return b;
}

static int stage5_magnetto(Enemy *e, int t) {
	TIMER(&t);

	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 5, ITEM_POWER, 5);
		return 1;
	}

	if(t < 0) {
		return 1;
	}

	cmplx offset = (frand()-0.5)*10 + (frand()-0.5)*10.0*I;
	iku_lightning_particle(e->pos + 3*offset, t);

	FROM_TO(0, 70, 1) {
		GO_TO(e, e->args[0], 0.1);
	}

	AT(140) {
		play_sound("redirect");
		play_sfx_delayed("redirect", 0, false, 180);
	}

	FROM_TO(140, 320, 1) {
		e->pos += 3 * cexp(I*(carg(e->args[1] - e->pos) + M_PI/2));
		GO_TO(e, e->args[1], pow((t - 140) / 300.0, 3));
	}

	FROM_TO_SND("shot1_loop", 140, 280, 1 + 2 * (1 + D_Lunatic - global.diff)) {
		// complex dir = cexp(I*carg(global.plr.pos - e->pos));

		for(int i = 0; i < 2 - (global.diff == D_Easy); ++i) {
			cmplx dir = cexp(I*(M_PI*i + M_PI/8*sin(2*(t-140)/70.0 * M_PI) + carg(e->args[1] - e->pos)));

			PROJECTILE(
				.proto = pp_ball,
				.pos = e->pos,
				.color = RGBA(0.1 + 0.5 * pow((t - 140) / 140.0, 2), 0.0, 0.8, 0.0),
				.rule = accelerated,
				.args = {
					(-2 + (global.diff == D_Hard)) * dir,
					0.02 * dir * (!i || global.diff != D_Lunatic),
				},
			);
		}
	}

	AT(380) {
		return ACTION_DESTROY;
	}

	return 1;
}

static int stage5_superbullet(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 4, ITEM_POWER, 3);
		return 1;
	}

	FROM_TO(0, 70, 1) {
		GO_TO(e, e->pos0 + e->args[0] * 70, 0.05);
	}

	FROM_TO(60, 200, 1) {
		cmplx n = cexp(I*M_PI*sin(_i/(8.0+global.diff)+frand()*0.1)+I*carg(global.plr.pos-e->pos));
		PROJECTILE(
			.proto = pp_bullet,
			.pos = e->pos + 50*n,
			.color = RGB(0.6, 0, 0),
			.rule = asymptotic,
			.args = { 2*n, 10 }
		);
		play_sound("shot1");
	}

	FROM_TO(260, 400, 1)
		e->pos -= e->args[0];
	return 1;
}

TASK(greeter_fairy, {
		cmplx pos;
		MoveParams move_enter;
		MoveParams move_exit;
}) {
	Enemy *e = TASK_BIND(espawn_fairy_blue_box(ARGS.pos, ITEMS(.points = 1)));
	e->move = ARGS.move_enter;
	real speed = difficulty_value(3.5, 3.5, 3.5, 4.5);
	real count = difficulty_value(1, 2, 3, 4);

	WAIT(80);
	for (int x = 0; x < 5; x++) {
		cmplx dir = cnormalize(global.plr.pos - e->pos);
		for (int i = -count; i <= count; i++) {
			PROJECTILE(
				.proto = pp_bullet,
				.pos = e->pos,
				.color = RGB(0.0, 0.0, 1.0),
				.move = move_asymptotic_simple(speed * dir * cdir(0.06 * i), 5),
			);
			play_sfx("shot1");
		}
		WAIT(20);
	}

	WAIT(20);

	e->move = ARGS.move_exit;
}

TASK(greeter_fairies_3, {
	int num;
}) {
	for (int i = 0; i < ARGS.num; i++) {
		cmplx pos = VIEWPORT_W * (i % 2) + 120 * I;
		real xdir = 1 - 2 * (i % 2);

		INVOKE_TASK(greeter_fairy,
			.pos = pos,
			.move_enter = move_towards(pos + 6 * (30 - 60 * (i % 2)) + I, 0.05),
			.move_exit = move_linear(3 * xdir)
		);
		WAIT(60);
	}
}


TASK(greeter_fairies_2, {
	int num;
}) {
	for (int i = 0; i < ARGS.num; i++) {
		cmplx pos = VIEWPORT_W * (i % 2) + 100.0 * I;
		real xdir = 1 - 2 * (i % 2);

		INVOKE_TASK(greeter_fairy,
			.pos = pos,
			.move_enter = move_towards(pos + (150 - 300 * (i % 2)), 0.05),
			.move_exit = move_linear(3 * xdir)
		);
		WAIT(80);
	}
}

TASK(greeter_fairies_1, {
	int num;
}) {
	for (int i = 0; i < ARGS.num; i++) {
		cmplx pos = VIEWPORT_W * (i % 2) + 70.0 * I + 50 * i * I;
		real xdir = 1 - 2 * (i % 2);

		INVOKE_TASK(greeter_fairy,
			.pos = pos,
			.move_enter = move_towards(pos + (150 - 300 * (i % 2)), 0.05),
			.move_exit = move_linear(3 * xdir)
		);
		WAIT(15);
	}
}

TASK(lightburst_fairy_move, {
	BoxedEnemy e;
	MoveParams move;
}) {
	Enemy *e = TASK_BIND(ARGS.e);
	e->move = ARGS.move;
}

TASK(lightburst_fairy_2, {
		cmplx pos;
		MoveParams move_enter;
		MoveParams move_exit;
}) {
	Enemy *e = TASK_BIND(espawn_big_fairy(ARGS.pos, ITEMS(.points = 4, .power = 2)));

	e->move = ARGS.move_enter;
	INVOKE_SUBTASK_DELAYED(200, lightburst_fairy_move, {
			.e = ENT_BOX(e),
			.move = ARGS.move_exit
	});

	real count = difficulty_value(6, 7, 8, 9);
	WAIT(70);
	for (int x = 0; x < 70; x++) {
		play_sfx("shot1_loop");
		for (int i = 0; i < count; i++) {
			cmplx n = cdir(carg(global.plr.pos) + 2.0 * M_PI / count * i);
			PROJECTILE(
				.proto = pp_bigball,
				.pos = e->pos + 50 * n * cdir(-1.0 * x * global.diff),
				.color = RGB(0.3, 0, 0.7 + 0.3 * (i % 2)),
				.move = move_asymptotic_simple(2.5 * n + 0.25 * global.diff * rng_sreal() * cdir(2.0 * M_PI * rng_sreal()), 3),
			);
			play_sfx("shot1_loop");
		}
		play_sfx("shot2");
		WAIT(5);
	}
}

TASK(lightburst_fairy_1, {
		cmplx pos;
		MoveParams move_enter;
		MoveParams move_exit;
}) {
	Enemy *e = TASK_BIND(espawn_big_fairy(ARGS.pos, ITEMS(.points = 4, .power = 2)));

	e->move = ARGS.move_enter;
	INVOKE_SUBTASK_DELAYED(200, lightburst_fairy_move, {
			.e = ENT_BOX(e),
			.move = ARGS.move_exit
	});

	real count = difficulty_value(6, 7, 8, 9);
	WAIT(70);
	for (int x = 0; x < 150; x++) {
		play_sfx("shot1_loop");
		for (int i = 0; i < count; i++) {
			cmplx n = cdir(carg(global.plr.pos) + 2.0 * M_PI / count * i);
			PROJECTILE(
				.proto = pp_ball,
				.pos = e->pos + 50 * n * cdir(-0.4 * x * global.diff),
				.color = RGB(0.3, 0, 0.7),
				.move = move_asymptotic_simple(3 * n, 3),
			);
			play_sfx("shot1_loop");
		}
		play_sfx("shot2");
		WAIT(5);
	}
}


TASK(lightburst_fairies_1, {
	int num;
	cmplx pos1;
	cmplx pos2;
	cmplx exit;
}) {
	for (int i = 0; i < ARGS.num; i++) {
		cmplx pos = ARGS.pos1 + ARGS.pos2 * i;
		INVOKE_TASK(lightburst_fairy_1,
			.pos = pos,
			.move_enter = move_towards(pos + ARGS.exit * 70 , 0.05),
			.move_exit = move_linear(ARGS.exit)
		);
		WAIT(40);
	}
}

TASK(lightburst_fairies_2, {
	int num;
	cmplx pos1;
	cmplx pos2;
	cmplx exit;
}) {
	for (int i = 0; i < ARGS.num; i++) {
		cmplx pos = ARGS.pos1 + ARGS.pos2 * i;
		INVOKE_TASK(lightburst_fairy_2,
			.pos = pos,
			.move_enter = move_towards(pos + ARGS.exit * 70 , 0.05),
			.move_exit = move_linear(ARGS.exit)
		);
		WAIT(40);
	}
}

TASK(loop_swirl_move, {
    MoveParams *move;
	cmplx turn;
	cmplx retention;
}) {
	int begin = creal(ARGS.turn) + 1;
	int end = cimag(ARGS.turn) - 1;
	int time = end - begin;

	ARGS.move->retention = ARGS.retention;
	WAIT(time / 1.5);
	ARGS.move->retention = 1;
}

TASK(loop_swirl, {
	cmplx start;
	cmplx velocity;
	cmplx turn;
	cmplx retention;
}) {
	Enemy *e = TASK_BIND(espawn_swirl(ARGS.start, ITEMS(.points = 4, .power = 2)));

    e->move = move_linear(ARGS.velocity);
    INVOKE_SUBTASK_DELAYED(80, loop_swirl_move,
        .move = &e->move,
		.turn = ARGS.turn,
		.retention = ARGS.retention
    );

	int difficulty = 30 - difficulty_value(4, 8, 16, 24);

	for (int x = 0; x < 200; x++) {
		for(int i = 1; i >= -1; i -= 2) {
			PROJECTILE(
				.proto = pp_bullet,
				.pos = e->pos,
				.color = RGB(0.3, 0.4, 0.5),
				.move = move_asymptotic_simple(i * 2 * ARGS.velocity * I / cabs(ARGS.velocity), 3),
				// TODO: bullets only fire down-left with the above
				// they should also fire in other directions correlating to the angle of "turn" the swirls make
				//.rule = asymptotic,
				//.args = { i*2*e->args[0]*I/cabs(e->args[0]), 3 }
			);
		}

		play_sound("shot1");
		WAIT(difficulty);
	}
}

TASK(loop_swirls, {
	int num;
	int direction;
	cmplx start;
	cmplx velocity;
	cmplx turn;
	cmplx retention;
}) {
	for (int i = 0; i < ARGS.num; i++) {
		INVOKE_TASK(loop_swirl, {
			.start = VIEWPORT_W * ARGS.direction + ARGS.start * rng_sreal(),
			.velocity = ARGS.velocity,
			.turn = ARGS.turn * rng_sreal() + ARGS.start,
			.retention = ARGS.retention,
		});
		WAIT(10);
	}
}

TASK(limiter_fairy, {
		cmplx pos;
		MoveParams exit;
}) {
	Enemy *e = TASK_BIND(espawn_fairy_red(ARGS.pos, ITEMS(.points = 4, .power = 4)));
	e->move = ARGS.exit;

	int difficulty = difficulty_value(0.25, 0.50, 0.75, 1);
	for (int x = 0; x < 400; x++) {
		double base_angle = carg(global.plr.pos - e->pos);

		for (int i = 1; i >= -1; i -= 2) {
			double a = i * 0.2 - 0.1 * difficulty + i * 3.0 / (x + 1);
			cmplx aim = cdir(base_angle + a);

			PROJECTILE(
				.proto = pp_rice,
				.pos = e->pos,
				.color = RGB(0.5, 0.1, 0.2),
				.move = move_asymptotic_simple(10 * aim, 2),
				.flags = PFLAG_NOSPAWNFADE,
			);
		}

		play_sfx("shot1_loop");
		WAIT(3);
	}
}

TASK(limiter_fairies, {
	int num;
	cmplx pos1;
	cmplx pos2;
	cmplx exit;
}) {
	for (int i = 0; i < ARGS.num; i++) {
		cmplx pos = ARGS.pos1 + ARGS.pos2 * (i % 2);
		INVOKE_TASK(limiter_fairy,
			.pos = pos,
			.exit = move_linear(ARGS.exit)
		);
		WAIT(60);
	}
}

TASK(miner_swirl, {
	cmplx start;
	cmplx velocity;
}) {
	Enemy *e = TASK_BIND(espawn_swirl(ARGS.start, ITEMS(.points = 2)));

	e->move = move_linear(ARGS.velocity);

	int difficulty = difficulty_value(8, 6, 4, 3);

	for (int x = 0; x < 600; x++) {
		PROJECTILE(
			.proto = pp_rice,
			.pos = e->pos + 20 * cdir(2.0 * M_PI * rng_real()),
			.color = RGB(0, 0, 255),
			.move = move_linear(cdir(2.0 * M_PI * rng_real())),
		);
		play_sfx_ex("shot3", 0, false);
		WAIT(difficulty);
	}
}

TASK(miner_swirls_3, {
	int num;
	cmplx start;
	cmplx velocity;
}) {
	for (int x = 0; x < ARGS.num; x++) {
		INVOKE_TASK(miner_swirl, {
			.start = 32 + (VIEWPORT_W - 2 * 32) * (x % 2) + VIEWPORT_H * I,
			.velocity = ARGS.velocity,
		});

		WAIT(60);
	}
}

TASK(miner_swirls_2, {
	int num;
	int time;
	cmplx start;
	cmplx velocity;
}) {
	int delay = ARGS.time / ARGS.num;
	for (int x = 0; x < ARGS.num; x++) {
		INVOKE_TASK(miner_swirl, {
			.start = ARGS.start * x,
			.velocity = ARGS.velocity,
		});

		WAIT(delay);
	}
}


TASK(miner_swirls_1, {
	int num;
	int time;
	cmplx start;
	cmplx velocity;
}) {
	int delay = ARGS.time / ARGS.num;
	for (int x = 0; x < ARGS.num; x++) {
		INVOKE_TASK(miner_swirl, {
			.start = VIEWPORT_W + 200.0 * I * rng_real(),
			.velocity = ARGS.velocity,
		});

		WAIT(delay);
	}
}

DEFINE_EXTERN_TASK(stage5_timeline) {
	YIELD;

	// 60
	INVOKE_TASK_DELAYED(1060, greeter_fairies_1, {
		.num = 7,
	});

	// 270
	INVOKE_TASK_DELAYED(1270, lightburst_fairies_1, {
		.num = 2,
		.pos1 = VIEWPORT_W/4,
		.pos2 = VIEWPORT_W/2,
		.exit = 2.0 * I,
	});

	// 400
	INVOKE_TASK_DELAYED(4000, loop_swirls, {
		.num = 20,
		.start = 200.0 * I,
		.velocity = 4 + I,
		.turn = 9,
		.retention = cdir(-0.05),
		.direction = 0,
	});

	// 700
	INVOKE_TASK_DELAYED(2000, loop_swirls, {
		.num = 10,
		.start = 200 * I,
		.velocity = -4 + I,
		.turn = 9,
		.retention = cdir(0.05),
		.direction = 1,
	});

	// 700
	INVOKE_TASK_DELAYED(2000, limiter_fairies, {
		.num = 2 + (global.diff != D_Easy),
		.pos1 = VIEWPORT_W/4,
		.pos2 = VIEWPORT_W/2,
		.exit = I,
	});

	// 1400
	INVOKE_TASK_DELAYED(1400, miner_swirls_1, {
		.num = difficulty_value(15, 20, 25, 30),
		.start = 200.0 * I,
		.velocity = -3.0 + 2.0 * I,
	});

	// 1500
	INVOKE_TASK_DELAYED(1500, greeter_fairies_2, {
		.num = 11,
	});

	// 2200
	INVOKE_TASK_DELAYED(2200, miner_swirls_2, {
		.num = difficulty_value(7, 9, 11, 14),
		.time = 400,
		.start = VIEWPORT_W / (6 + global.diff),
		.velocity = 3.0 * I,
	});

	// 2500
	INVOKE_TASK_DELAYED(2500, lightburst_fairies_1, {
		.num = 1,
		.pos1 = VIEWPORT_W/2,
		.pos2 = 0,
		.exit = 2.0 * I,
	});

	// 2700
	if (global.diff > D_Easy) {
		INVOKE_TASK_DELAYED(2700, lightburst_fairies_1, {
			.num = 1,
			.pos1 = (VIEWPORT_W - 20) + (120 * I),
			.pos2 = 0,
			.exit = -2.0,
		});
	}

	// 3000
	INVOKE_TASK_DELAYED(3000, lightburst_fairies_2, {
		.num = 3,
		.pos1 = VIEWPORT_W/4,
		.pos2 = VIEWPORT_W/4,
		.exit = 2.0 * I,
	});

	// 4260
	INVOKE_TASK_DELAYED(4260, greeter_fairies_3, {
		.num = 13,
	});

	// 4320
	INVOKE_TASK_DELAYED(4320, miner_swirls_3, {
		.num = 2,
		.velocity = -I,
	});

	// 5000
	INVOKE_TASK_DELAYED(5000, lightburst_fairies_1, {
		.num = 1,
		.pos1 = VIEWPORT_W/2,
		.pos2 = 0,
		.exit = 2.0 * I,
	});

	// 5000
	INVOKE_TASK_DELAYED(5000, lightburst_fairies_1, {
		.num = 1,
		.pos1 = VIEWPORT_W/2,
		.pos2 = 0,
		.exit = 2.0 * I,
	});

	// 5180
	INVOKE_TASK_DELAYED(5180, lightburst_fairies_2, {
		.num = 1,
		.pos1 = VIEWPORT_W/2,
		.pos2 = 100,
		.exit = 2.0 * I - 0.25,
	});

	// 5500
	INVOKE_TASK_DELAYED(5500, lightburst_fairies_1, {
		.num = 1,
		.pos1 = VIEWPORT_W+20 + VIEWPORT_H * 0.6 * I,
		.pos2 = 0,
		.exit = -2 * I - 2,
	});

	// 5500
	INVOKE_TASK_DELAYED(5500, lightburst_fairies_1, {
		.num = 1,
		.pos1 = -20 + VIEWPORT_H * 0.6 * I,
		.pos2 = 0,
		.exit = -2 * I + 2,
	});

	// 5060
	INVOKE_TASK_DELAYED(5060, miner_swirls_1, {
		.num = difficulty_value(5, 7, 9, 12),
		.time = 240,
		.velocity = -3 + 2.0 * I,
	});

	// 6100
	INVOKE_TASK_DELAYED(6100, miner_swirls_1, {
		.num = difficulty_value(5, 10, 15, 20),
		.time = 250,
		.velocity = -3 + 2.0 * I,
	});

	// 6300
	INVOKE_TASK_DELAYED(6300, limiter_fairies, {
		.num = 1,
		.pos1 = VIEWPORT_W/4,
		.pos2 = VIEWPORT_W/2,
		.exit = 2 * I,
	});
}

void stage5_events(void) {
	TIMER(&global.timer);

	// TODO: ??????
	/* FROM_TO(200, 1000, 20-3*global.diff) { */
	/* 	float f = frand(); */
	/* 	create_enemy3c( */
	/* 		VIEWPORT_W/2+300*sin(global.frames)*cos(2*global.frames), */
	/* 		400, */
	/* 		Swirl, */
	/* 		stage5_swirl, */
	/* 		2*cexp(I*M_PI*f)+I, */
	/* 		60 + 100.0*I, */
	/* 		cexp(0.01*I*(1-2*(f<0.5))) */
	/* 	); */
	/* } */

	AT(1000)
		create_enemy1c(VIEWPORT_W/2, 9000, BigFairy, stage5_laserfairy, 2.0*I);

	AT(2900)
		global.boss = stage5_spawn_midboss();

	AT(2920) {
		stage5_dialog_post_midboss();

		// TODO: determine if this hack is still required with the new dialogue system
		global.timer++;
	}

	FROM_TO(3300, 4000, 90-10*global.diff)
		create_enemy1c(200.0*I+VIEWPORT_W*(_i&1), 1500, Fairy, stage5_superbullet, 3-6*(_i&1));

	AT(3400) {
		create_enemy2c(VIEWPORT_W/4, 6000, BigFairy, stage5_laserfairy, 2.0*I, 1);
		create_enemy2c(VIEWPORT_W/4*3, 6000, BigFairy, stage5_laserfairy, 2.0*I, 1);
	}



	FROM_TO(5030, 5060, 30) {
		create_enemy1c(30.0*I+VIEWPORT_W*(_i&1), 1500, Fairy, stage5_superbullet, 2-4*(_i&1) + I);
	}


	AT(5360) {
		create_enemy1c(30.0*I+VIEWPORT_W, 1500, Fairy, stage5_superbullet, -2 + I);

	}

	AT(5390) {
		create_enemy1c(30.0*I, 1500, Fairy, stage5_superbullet, 2 + I);
	}


	AT(5600) {
		enemy_kill_all(&global.enemies);
	}

	{
		int cnt = 5;
		int step = 10;
		double ofs = 42*2;

		FROM_TO(5620, 5620 + step*cnt-1, step) {
			cmplx src1 = -ofs/4               + (-ofs/4 +      _i    * (VIEWPORT_H-2*ofs)/(cnt-1))*I;
			cmplx src2 = (VIEWPORT_W + ofs/4) + (-ofs/4 + (cnt-_i-1) * (VIEWPORT_H-2*ofs)/(cnt-1))*I;
			cmplx dst1 = ofs                  + ( ofs   +      _i    * (VIEWPORT_H-2*ofs)/(cnt-1))*I;
			cmplx dst2 = (VIEWPORT_W - ofs)   + ( ofs   + (cnt-_i-1) * (VIEWPORT_H-2*ofs)/(cnt-1))*I;

			create_enemy2c(src1, 2000, Swirl, stage5_magnetto, dst1, dst2);
			create_enemy2c(src2, 2000, Swirl, stage5_magnetto, dst2, dst1);
		}
	}



	AT(6960) {
		stage_unlock_bgm("stage5");
		global.boss = stage5_spawn_boss();
	}

	AT(6980) {
		stage_unlock_bgm("stage5boss");
		stage5_dialog_post_boss();
	}

	AT(6985) {
		stage_finish(GAMEOVER_SCORESCREEN);
	}
}

