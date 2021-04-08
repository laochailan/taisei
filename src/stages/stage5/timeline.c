/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "move.h"
#include "taisei.h"

#include "timeline.h"
#include "stage5.h"
#include "nonspells/nonspells.h"
#include "spells/spells.h"

#include "coroutine.h"
#include "enemy_classes.h"

static void stage5_dialog_post_boss(void) {
	PlayerMode *pm = global.plr.mode;
	INVOKE_TASK_INDIRECT(Stage5PostBossDialog, pm->dialog->Stage5PostBoss);
}

static void stage5_dialog_post_midboss(void) {
	PlayerMode *pm = global.plr.mode;
	INVOKE_TASK_INDIRECT(Stage5PostMidBossDialog, pm->dialog->Stage5PostMidBoss);
}

static void midboss_dummy(Boss *b, int t) { }

TASK(magnetto_swirl_shoot, {
	BoxedEnemy e;
	cmplx move_to;
}) {
	Enemy *e = TASK_BIND(ARGS.e);

	int interval = difficulty_value(12, 9, 6, 3);
	int bullets = difficulty_value(1, 2, 2, 2);
	int bullet_dir = difficulty_value(0, 0, 1, 0);
	int bullet_dir2 = difficulty_value(1, 1, 1, 0);
	for(int t = 0; t <= 180; t += WAIT(interval)) {
		for(int i = 0; i < bullets; i++) {
			cmplx dir = cdir(M_PI * i + M_PI / 8 * sin(2 * t / 70.0 * M_PI)) * cnormalize(ARGS.move_to - e->pos);
			PROJECTILE(
				.proto = pp_ball,
				.pos = e->pos,
				.color = RGBA(0.1 + 0.5 * pow(t / 180.0, 2), 0.0, 0.8, 0.0),
				.move = move_accelerated((-2 + bullet_dir) * dir, 0.035 * dir * (!i || bullet_dir2)),
			);
		}
	}
}

TASK(magnetto_swirl_move, {
	BoxedEnemy e;
	cmplx move_to;
}) {
	Enemy *e = TASK_BIND(ARGS.e);

	cmplx swoop = 2.75 * cdir(M_PI/2);
	for(int t = 0; t <= 140; t++, YIELD) {
		if(!(t % 5)) {
			cmplx offset = rng_dir();
			iku_lightning_particle(e->pos + 15 * rng_real() * offset);
		}
		e->move = move_towards(ARGS.move_to, pow(t / 300.0, 3));
		e->move.acceleration = cnormalize(ARGS.move_to - e->pos) * swoop;
	}
	e->move = move_towards(ARGS.move_to, 0.5);
	WAIT(50);
	enemy_kill(e);
}

TASK(magnetto_swirl, {
	cmplx pos;
	MoveParams move_enter;
	cmplx move_to;
}) {
	Enemy *e = TASK_BIND(espawn_swirl(ARGS.pos, ITEMS(.points = 5, .power = 5)));
	e->move = ARGS.move_enter;

	WAIT(140);
	play_sfx("redirect");
	play_sfx_delayed("redirect", 0, false, 180);

	INVOKE_TASK(magnetto_swirl_move, {
		.e = ENT_BOX(e),
		.move_to = ARGS.move_to
	});
	INVOKE_TASK(magnetto_swirl_shoot, {
		.e = ENT_BOX(e),
		.move_to = ARGS.move_to
	});

}

TASK(magnetto_swirls, {
	int num;
}) {
	enemy_kill_all(&global.enemies);
	real ofs = 42 * 2;
	int count = ARGS.num - 1;
	for(int i = 0; i < ARGS.num; i++) {

		INVOKE_TASK(magnetto_swirl,
			.pos = -ofs / 4 + (-ofs / 4 + i * (VIEWPORT_H-2 * ofs) / count) * I, // src1 (e->pos)
			.move_enter = move_towards((ofs + (ofs + i * (VIEWPORT_H-2 * ofs) / count) * I), 0.1), // dst1 (e->args[0])
			.move_to = (VIEWPORT_W - ofs) + ( ofs + (count - i) * (VIEWPORT_H-2 * ofs) / count) * I // dst2 (e->args[1])
		);

		INVOKE_TASK(magnetto_swirl,
			.pos = (VIEWPORT_W + ofs / 4) + (-ofs / 4 + (count - i) * (VIEWPORT_H-2 * ofs) / count) * I,
			.move_enter = move_towards((VIEWPORT_W - ofs) + ( ofs + (count - i) * (VIEWPORT_H-2 * ofs) / count) * I, 0.1), // dst2 (e->args[0])
			.move_to = ofs + (ofs + i * (VIEWPORT_H-2 * ofs) / count) * I // dst1 (e->args[1])
		);
		WAIT(20);
	}
}

TASK(spawn_midboss, NO_ARGS) {
	STAGE_BOOKMARK(midboss);
	Boss *boss = global.boss = create_boss("Bombs?", "iku_mid", VIEWPORT_W + 800.0 * I);
	boss->glowcolor = *RGB(0.2, 0.4, 0.5);
	boss->shadowcolor = *RGBA_MUL_ALPHA(0.65, 0.2, 0.75, 0.5);

	Attack *a = boss_add_attack_from_info(boss, &stage5_spells.mid.static_bomb, false);
	boss_set_attack_bonus(a, 5);

	// suppress the boss death effects (this triggers the "boss fleeing" case)
	// TODO: investigate why the death effect doesn't work (points/items still spawn off-screen)
	boss_add_attack(boss, AT_Move, "", 0, 0, midboss_dummy, NULL);
	boss_engage(boss);
}

TASK(boss_appear, { BoxedBoss boss; }) {
	Boss *boss = TASK_BIND(ARGS.boss);
	boss->move = move_towards(VIEWPORT_W/2 + 240.0 * I, 0.015);
}

TASK(spawn_boss, NO_ARGS) {
	STAGE_BOOKMARK_DELAYED(120, boss);
	Boss *boss = global.boss = stage5_spawn_iku(VIEWPORT_W/2 - 200.0 * I);
	PlayerMode *pm = global.plr.mode;
	Stage5PreBossDialogEvents *e;

	INVOKE_TASK_INDIRECT(Stage5PreBossDialog, pm->dialog->Stage5PreBoss, &e);
	INVOKE_TASK_WHEN(&e->boss_appears, boss_appear, ENT_BOX(boss));
	INVOKE_TASK_WHEN(&e->music_changes, common_start_bgm, "stage3boss");
	WAIT_EVENT(&global.dialog->events.fadeout_began);

	boss_add_attack_task(boss, AT_Normal, "Bolts1", 40, 24000, TASK_INDIRECT(BossAttack, stage5_boss_nonspell_1), NULL);
	boss_add_attack_from_info(boss, &stage5_spells.boss.atmospheric_discharge, false);

	boss_add_attack_task(boss, AT_Normal, "Bolts2", 45, 27000, TASK_INDIRECT(BossAttack, stage5_boss_nonspell_2), NULL);
	boss_add_attack_from_info(boss, &stage5_spells.boss.artificial_lightning, false);

	boss_add_attack_task(boss, AT_Normal, "Bolts3", 50, 30000, TASK_INDIRECT(BossAttack, stage5_boss_nonspell_3), NULL);
	if(global.diff < D_Hard) {
		boss_add_attack_from_info(boss, &stage5_spells.boss.induction_field, false);
	} else {
		boss_add_attack_from_info(boss, &stage5_spells.boss.inductive_resonance, false);
	}
	boss_add_attack_from_info(boss, &stage5_spells.boss.natural_cathode, false);

	boss_add_attack_from_info(boss, &stage5_spells.extra.overload, false);

	boss_engage(boss);
}

TASK(greeter_fairy, {
	cmplx pos;
	MoveParams move_enter;
	MoveParams move_exit;
}) {
	Enemy *e = TASK_BIND(espawn_fairy_blue(ARGS.pos, ITEMS(.points = 1)));
	e->move = ARGS.move_enter;
	real speed = difficulty_value(3.5, 3.5, 3.5, 4.5);
	real count = difficulty_value(1, 2, 3, 4);

	WAIT(80);
	for(int x = 0; x < 5; x++) {
		cmplx dir = cnormalize(global.plr.pos - e->pos);
		for(int i = -count; i <= count; i++) {
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
	for(int i = 0; i < ARGS.num; i++) {
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
	for(int i = 0; i < ARGS.num; i++) {
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
	for(int i = 0; i < ARGS.num; i++) {
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
	int difficulty = difficulty_value(1, 2, 3, 4);
	WAIT(70);
	for(int x = 0; x < 70; x++) {
		play_sfx("shot1_loop");
		for(int i = 0; i < count; i++) {
			cmplx n = cnormalize(global.plr.pos - e->pos) * cdir(i * M_TAU / count);
			PROJECTILE(
				.proto = pp_bigball,
				.pos = e->pos + 50 * n * cdir(-1.0 * x * difficulty),
				.color = RGB(0.3, 0, 0.7 + 0.3 * (i % 2)),
				.move = move_asymptotic_simple(2.5 * n + 0.25 * difficulty * rng_sreal() * rng_dir(), 3),
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
	int difficulty = difficulty_value(1, 2, 3, 4);
	WAIT(70);
	for(int x = 0; x < 150; x++) {
		play_sfx("shot1_loop");
		for(int i = 0; i < count; i++) {
			cmplx n = cnormalize(global.plr.pos - e->pos) * cdir(i * M_TAU / count);
			PROJECTILE(
				.proto = pp_ball,
				.pos = e->pos + 50 * n * cdir(-0.4 * x * difficulty),
				.color = RGB(0.3, 0, 0.7),
				.move = move_asymptotic_simple(3 * n, 3),
			);
			play_sfx("shot1_loop");
		}
		play_sfx("shot2");
		WAIT(5);
	}
}

TASK(laser_fairy, {
	cmplx pos;
	MoveParams move_enter;
	MoveParams move_exit;
	int reduction;
}) {
	Enemy *e = TASK_BIND(espawn_big_fairy(ARGS.pos, ITEMS(.points = 4, .power = 2)));

	e->move = ARGS.move_enter;
	WAIT(100);

	int delay = difficulty_value(6, 5, 4, 3) + ARGS.reduction;
	int amount = 700 / delay;
	int difficulty = difficulty_value(1, 2, 3, 4);

	for(int x = 0; x < amount; x++) {
		cmplx n = cnormalize(global.plr.pos - e->pos) * cdir((0.2 - 0.02 * difficulty) * x);
		real fac = (0.5 + 0.2 * difficulty);

		// TODO: is this the correct "modern" way of invoking lasers?
		create_lasercurve2c(e->pos, 100, 300, RGBA(0.7, 0.3, 1, 0), las_accel, fac * 4 * n, fac * 0.05 * n);

		PROJECTILE(
			.proto = pp_plainball,
			.pos = e->pos,
			.color = RGBA(0.7, 0.3, 1, 0),
			.move = move_accelerated(fac * 4 * n, fac * 0.05 * n),
		);
		play_sfx_ex("shot_special1", 0, true);
		WAIT(delay);
	}

	WAIT(30);
	e->move = ARGS.move_exit;
}

TASK(lightburst_fairies_1, {
	int num;
	cmplx pos;
	cmplx offset;
	cmplx exit;
}) {
	for(int i = 0; i < ARGS.num; i++) {
		cmplx pos = ARGS.pos + ARGS.offset * i;
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
	cmplx pos;
	cmplx offset;
	cmplx exit;
}) {
	for(int i = 0; i < ARGS.num; i++) {
		cmplx pos = ARGS.pos + ARGS.offset * i;
		INVOKE_TASK(lightburst_fairy_2,
			.pos = pos,
			.move_enter = move_towards(pos + ARGS.exit * 70 , 0.05),
			.move_exit = move_linear(ARGS.exit)
		);
		WAIT(40);
	}
}

TASK(loop_swirl, {
	cmplx start;
	cmplx velocity;
	real turn_angle;
	int turn_duration;
	int turn_delay;
}) {
	Enemy *e = TASK_BIND(espawn_swirl(ARGS.start, ITEMS(.points = 4, .power = 2)));

	e->move = move_linear(ARGS.velocity);
	INVOKE_SUBTASK(common_move_turn,
		.move_params = &e->move,
		.turn_angle = ARGS.turn_angle,
		.turn_duration = ARGS.turn_duration,
		.turn_delay = ARGS.turn_delay
    );

	int difficulty = 30 - difficulty_value(4, 8, 16, 24);

	for(int x = 0; x < 200; x++) {
		for(int i = 1; i >= -1; i -= 2) {
			PROJECTILE(
				.proto = pp_bullet,
				.pos = e->pos,
				.color = RGB(0.3, 0.4, 0.5),
				.move = move_asymptotic_simple(i * 2 * I / cnormalize(e->move.velocity), 3),
			);
		}

		play_sfx("shot1");
		WAIT(difficulty);
	}
}

TASK(loop_swirls, {
	int num;
	int direction;
	cmplx start;
	cmplx velocity;
	int turn_delay;
	int turn_duration;
	real turn_angle;
}) {
	for(int i = 0; i < ARGS.num; i++) {
		INVOKE_TASK(loop_swirl, {
			.start = VIEWPORT_W * ARGS.direction + ARGS.start * rng_sreal(),
			.velocity = ARGS.velocity,
			.turn_angle = ARGS.turn_angle,
			.turn_duration = ARGS.turn_duration,
			.turn_delay = ARGS.turn_delay,
		});
		WAIT(10);
	}
}

TASK(loop_swirls_2, {
	int num;
	int duration;
}) {
	for(int i = 0; i < ARGS.num; i++) {
		real f = rng_real();
		real xdir = 1 - 2 * (i % 2);
		INVOKE_TASK(loop_swirl, {
			.start = VIEWPORT_W/2 + 300 * sin(i * 20) * cos (2 * i * 20),
			.velocity = 2 * cdir(M_PI * f) + I,
			.turn_angle = M_PI/2 * xdir,
			.turn_duration = 50,
			.turn_delay = 100,
		});
		WAIT(ARGS.duration / ARGS.num);
	}
}

TASK(limiter_fairy, {
	cmplx pos;
	MoveParams exit;
}) {
	Enemy *e = TASK_BIND(espawn_fairy_red(ARGS.pos, ITEMS(.points = 4, .power = 4)));
	e->move = ARGS.exit;

	int difficulty = difficulty_value(0.25, 0.50, 0.75, 1);
	for(int x = 0; x < 400; x++) {

		for(int i = 1; i >= -1; i -= 2) {
			real a = i * 0.2 - 0.1 * difficulty + i * 3.0 / (x + 1);
			cmplx plraim = cnormalize(global.plr.pos - e->pos);
			cmplx aim = plraim * cdir(a);

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
	cmplx pos;
	cmplx offset;
	cmplx exit;
}) {
	for(int i = 0; i < ARGS.num; i++) {
		cmplx pos = ARGS.pos + ARGS.offset * (i % 2);
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

	for(int x = 0; x < 600; x++) {
		PROJECTILE(
			.proto = pp_rice,
			.pos = e->pos + 20 * cdir(M_TAU * rng_real()),
			.color = RGB(0, 0, 255),
			.move = move_linear(cdir(M_TAU  * rng_real())),
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
	for(int x = 0; x < ARGS.num; x++) {
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
	for(int x = 0; x < ARGS.num; x++) {
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
	for(int x = 0; x < ARGS.num; x++) {
		INVOKE_TASK(miner_swirl, {
			.start = VIEWPORT_W + 200.0 * I * rng_real(),
			.velocity = ARGS.velocity,
		});
		WAIT(delay);
	}
}

TASK(superbullet_fairy, {
	cmplx pos;
	cmplx acceleration;
	real offset;
}) {
	Enemy *e = TASK_BIND(espawn_fairy_blue(ARGS.pos, ITEMS(.points = 4, .power = 2)));

	e->move = move_towards(e->pos + ARGS.acceleration * 70 + ARGS.offset, 0.05);
	WAIT(60);

	float difficulty = difficulty_value(9.0, 10.0, 11.0, 12.0);
	for(int x = 0; x < 140; x++, YIELD) {
		cmplx n = cdir(M_PI * sin(x / difficulty + rng_sreal() * 0.1)) * cnormalize(global.plr.pos - e->pos);
		PROJECTILE(
			.proto = pp_bullet,
			.pos = e->pos + 50 * n,
			.color = RGB(0.6, 0, 0),
			.move = move_asymptotic_simple(2 * n, 10),
		);
		play_sfx("shot1");
	}

	WAIT(60);
	e->move = move_linear(-ARGS.acceleration);
}

TASK(superbullet_fairies_1, {
	int num;
	int time;
}) {
	int delay = ARGS.time / ARGS.num;
	for(int i = 0; i < ARGS.num; i++) {
		real offset = -15 * i * (1 - 2 * (i % 2));
		INVOKE_TASK(superbullet_fairy, {
			.pos = VIEWPORT_W * (i % 2) + 100.0 * I + 15 * i * I,
			.acceleration = 3 - 6 * (i % 2),
			.offset = offset,
		});
		WAIT(delay);
	}
}

DEFINE_EXTERN_TASK(stage5_timeline) {
	// 60
	INVOKE_TASK_DELAYED(60, greeter_fairies_1, {
		.num = 7,
	});

	// 270
	INVOKE_TASK_DELAYED(270, lightburst_fairies_1, {
		.num = 2,
		.pos = VIEWPORT_W/4,
		.offset = VIEWPORT_W/2,
		.exit = 2.0 * I,
	});

	// 400
	INVOKE_TASK_DELAYED(400, loop_swirls, {
		.num = 20,
		.start = 200.0 * I,
		.velocity = 4 + I,
		.turn_angle = M_TAU,
		.turn_delay = 80,
		.turn_duration = 100,
	});

	// 700
	INVOKE_TASK_DELAYED(700, loop_swirls, {
		.num = 10,
		.start = 200 * I,
		.velocity = -4 + I,
		.turn_angle = M_TAU,
		.turn_delay = 80,
		.turn_duration = 100,
		.direction = 1,
	});

	// 870/920
	INVOKE_TASK_DELAYED(difficulty_value(920, 870, 870, 870), limiter_fairies, {
		.num = difficulty_value(2, 3, 3, 3),
		.pos = VIEWPORT_W/4,
		.offset = VIEWPORT_W/2,
		.exit = I,
	});

	// 1000
	INVOKE_TASK_DELAYED(1000, laser_fairy, {
		.pos = VIEWPORT_W/2,
		.move_enter = move_towards(VIEWPORT_W/2 + 2.0 * I * 100, 0.05),
		.move_exit = move_linear(-(2.0 * I)),
		.reduction = 1,
	});

	// 1400
	int miner_time = difficulty_value(1700, 1600, 1500, 1400);
	INVOKE_TASK_DELAYED(miner_time, miner_swirls_1, {
		.num = difficulty_value(15, 20, 25, 30),
		.time = 2560 - miner_time,
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
		.pos = VIEWPORT_W/2,
		.offset = 0,
		.exit = 2.0 * I,
	});

	// 2700
	if(global.diff > D_Easy) {
		INVOKE_TASK_DELAYED(2700, lightburst_fairies_1, {
			.num = 1,
			.pos = (VIEWPORT_W - 20) + (120 * I),
			.offset = 0,
			.exit = -2.0,
		});
	}

	STAGE_BOOKMARK_DELAYED(2900, pre-midboss);
	INVOKE_TASK_DELAYED(2900, spawn_midboss);
	while(!global.boss) YIELD;
	STAGE_BOOKMARK(post-midboss);
	WAIT(1200);
	stage5_dialog_post_midboss();
	WAIT(200);

	// 3000
	INVOKE_TASK_DELAYED(0, lightburst_fairies_2, {
		.num = 3,
		.pos = VIEWPORT_W/4,
		.offset = VIEWPORT_W/4,
		.exit = 2.0 * I,
	});

	// 3300
	INVOKE_TASK_DELAYED(300, superbullet_fairies_1, {
		.num = difficulty_value(8, 10, 12, 14),
		.time = 700,
	});

	// 3400
	INVOKE_TASK_DELAYED(400, laser_fairy, {
		.pos = VIEWPORT_W/4,
		.move_enter = move_towards(VIEWPORT_W/4 + 2.0 * I * 100, 0.05),
		.move_exit = move_linear(-(2.0 * I)),
		.reduction = 2,
	});

	// 3400
	INVOKE_TASK_DELAYED(400, laser_fairy, {
		.pos = VIEWPORT_W/4 * 3,
		.move_enter = move_towards(VIEWPORT_W/4 * 3 + 2.0 * I * 100, 0.05),
		.move_exit = move_linear(-(2.0 * I)),
		.reduction = 2,
	});

	// 4200
	INVOKE_TASK_DELAYED(1200, loop_swirls_2, {
		.num = difficulty_value(47, 53, 57, 60),
		.duration = 800,
	});

	// 4260
	INVOKE_TASK_DELAYED(1260, greeter_fairies_3, {
		.num = 13,
	});

	// 4320
	INVOKE_TASK_DELAYED(1320, miner_swirls_3, {
		.num = 2,
		.velocity = -I,
	});

	// 5000
	INVOKE_TASK_DELAYED(2000, lightburst_fairies_1, {
		.num = 1,
		.pos = VIEWPORT_W/2,
		.offset = 0,
		.exit = 2.0 * I,
	});

	// 5000
	INVOKE_TASK_DELAYED(2000, lightburst_fairies_1, {
		.num = 1,
		.pos = VIEWPORT_W/2,
		.offset = 0,
		.exit = 2.0 * I,
	});

	// 5030
	INVOKE_TASK_DELAYED(2030, superbullet_fairy, {
		.pos = 30.0 * I + VIEWPORT_W,
		.acceleration = -2 + I,
	});

	// 5060
	INVOKE_TASK_DELAYED(2060, superbullet_fairy, {
		.pos = 30.0 * I,
		.acceleration = 2 + I,
	});

	// 5060
	INVOKE_TASK_DELAYED(2060, miner_swirls_1, {
		.num = difficulty_value(5, 7, 9, 12),
		.time = 240,
		.velocity = -3 + 2.0 * I,
	});

	// 5180
	INVOKE_TASK_DELAYED(2180, lightburst_fairies_2, {
		.num = 1,
		.pos = VIEWPORT_W/2,
		.offset = 100,
		.exit = 2.0 * I - 0.25,
	});

	// 5360
	INVOKE_TASK_DELAYED(2360, superbullet_fairy, {
		.pos = 30.0 * I + VIEWPORT_W,
		.acceleration = -2 + I,
	});

	// 5390
	INVOKE_TASK_DELAYED(2390, superbullet_fairy, {
		.pos = 30.0 * I,
		.acceleration = 2 + I,
	});

	// 5500
	INVOKE_TASK_DELAYED(2500, lightburst_fairies_1, {
		.num = 1,
		.pos = VIEWPORT_W+20 + VIEWPORT_H * 0.6 * I,
		.offset = 0,
		.exit = -2 * I - 2,
	});

	// 5500
	INVOKE_TASK_DELAYED(2500, lightburst_fairies_1, {
		.num = 1,
		.pos = -20 + VIEWPORT_H * 0.6 * I,
		.offset = 0,
		.exit = -2 * I + 2,
	});

	// 5600
	INVOKE_TASK_DELAYED(2600, magnetto_swirls, {
		.num = 5,
	});

	// 6100
	INVOKE_TASK_DELAYED(3100, miner_swirls_1, {
		.num = difficulty_value(5, 10, 15, 20),
		.time = 250,
		.velocity = -3 + 2.0 * I,
	});

	// 6300
	INVOKE_TASK_DELAYED(3300, limiter_fairies, {
		.num = 1,
		.pos = VIEWPORT_W/4,
		.offset = VIEWPORT_W/2,
		.exit = 2 * I,
	});

	STAGE_BOOKMARK_DELAYED(3400, pre-boss);

	WAIT(3700);
	INVOKE_TASK(spawn_boss);
	while(!global.boss) YIELD;
	WAIT_EVENT(&global.boss->events.defeated);

	stage_unlock_bgm("stage5boss");

	WAIT(240);
	stage5_dialog_post_boss();
	WAIT_EVENT(&global.dialog->events.fadeout_began);

	WAIT(5);
	stage_finish(GAMEOVER_SCORESCREEN);
}
