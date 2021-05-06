/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "timeline.h"
#include "stage3.h"
#include "wriggle.h"
#include "scuttle.h"
#include "spells/spells.h"
#include "nonspells/nonspells.h"
#include "background_anim.h"

#include "global.h"
#include "stagetext.h"
#include "common_tasks.h"
#include "enemy_classes.h"

MODERNIZE_THIS_FILE_AND_REMOVE_ME

static void stage3_dialog_post_boss(void) {
	PlayerMode *pm = global.plr.mode;
	INVOKE_TASK_INDIRECT(Stage3PostBossDialog, pm->dialog->Stage3PostBoss);
}

TASK(destroy_enemy, { BoxedEnemy e; }) {
	// used for when enemies should pop after a preset time
	enemy_kill(TASK_BIND(ARGS.e));
}

TASK(death_burst, { BoxedEnemy e; ProjPrototype *shot_proj; }) {
	// explode in a hail of bullets
	Enemy *e = TASK_BIND(ARGS.e);

	float r, g;
	if(rng_chance(0.5)) {
		r = 0.3;
		g = 1.0;
	} else {
		r = 1.0;
		g = 0.3;
	}

	int intensity = difficulty_value(12, 16, 20, 24);
	for(int i = 0; i < intensity; ++i) {
		real a = (M_PI * 2.0 * i) / intensity;
		cmplx dir = cdir(a);

		PROJECTILE(
			.proto = ARGS.shot_proj, // pp_ball or pp_rice,
			.pos = e->pos,
			.color = RGB(r, g, 1.0),
			.move = move_asymptotic_simple(1.5 * dir, 10 - 10 * psin(2 * a + M_PI/2)),
		);
	}

}

TASK(burst_swirl, { cmplx pos; cmplx dir; ProjPrototype *shot_proj; }) {
	// swirls - fly in, explode when killed or after a preset time
	Enemy *e = TASK_BIND(espawn_swirl(ARGS.pos, ITEMS(
		.points = 1,
		.power = 1,
	)));

	// die after preset time
	INVOKE_TASK_DELAYED(60, destroy_enemy, ENT_BOX(e));
	INVOKE_TASK_WHEN(&e->events.killed, death_burst, ENT_BOX(e), ARGS.shot_proj);

	e->move = move_linear(ARGS.dir);
}

TASK(burst_swirls, { int count; int interval; ProjPrototype *shot_proj; }) {
	for(int i = 0; i < ARGS.count; ++i) {
		// spawn in a "pitchfork" pattern
		cmplx pos = VIEWPORT_W/2 + 20 * rng_sreal();
		pos += (VIEWPORT_H/4 + 20 * rng_sreal()) * I;
		INVOKE_TASK(burst_swirl,
			.pos = pos,
			.dir = 3 * (I + sin(M_PI*global.frames/15.0)),
			.shot_proj = ARGS.shot_proj
		);
		WAIT(ARGS.interval);
	}
}

// side swirls
// typically move across a stage in a drive-by fashion

TASK(side_swirl, { MoveParams move; cmplx start_pos; }) {
	Enemy *e = TASK_BIND(espawn_swirl(ARGS.start_pos, ITEMS(
		.points = 1,
		.power = 1,
	)));

	e->move = ARGS.move;
	WAIT(50);

	int intensity = difficulty_value(1, 2, 3, 4);

	for(int i = 0; i < intensity; ++i ) {
		PROJECTILE(
			.proto = pp_flea,
			.pos = e->pos,
			.color = RGB(0.7, 0.0, 0.5),
			.move = move_accelerated(
				2 * (cnormalize(global.plr.pos - e->pos)),
				0.005 * rng_dir() * intensity
			),
		);

		play_sfx("shot1");
		WAIT(20);
	}

}

TASK(side_swirls_procession, { int interval; int count; MoveParams move; cmplx start_pos; }) {
	for(int x = 0; x < ARGS.count; ++x) {
		INVOKE_TASK(side_swirl, ARGS.move, ARGS.start_pos);
		WAIT(ARGS.interval);
	}
}

TASK(little_fairy_shot_ball, { BoxedEnemy e; int shot_interval; int intensity;} ) {
	Enemy *e = TASK_BIND(ARGS.e);

	e->move.attraction = 0.05;

	WAIT(60);

	for (int i = 0; i < ARGS.intensity; ++i) {
		float a = global.timer * 0.5;
		cmplx dir = cdir(a);

		// fire out danmaku in all directions in a spiral-ish pattern
		PROJECTILE(
			.proto = pp_wave,
			.pos = e->pos + dir * 10,
			.color = (i % 2) ? RGB(1.0, 0.3, 0.3) : RGB(0.3, 0.3, 1.0),
			.move = move_accelerated(dir, dir * 0.025),
		);

		// danmaku that only shows up above Easy difficulty
		if(global.diff > D_Easy) {
			PROJECTILE(
				.proto = pp_ball,
				.pos = e->pos + dir * 10,
				.color = RGB(1.0, 0.6, 0.3),
				.move = move_linear(dir * (1.0 + 0.5 * sin(a))),
			);
		}

		play_sfx("shot1");
		WAIT(ARGS.shot_interval);
	}

	WAIT(60);
}

TASK(little_fairy_shot_wave, { BoxedEnemy e; int shot_interval; int intensity; } ) {
	Enemy *e = TASK_BIND(ARGS.e);
	e->move.attraction = 0.03;
	WAIT(60);

	for (int i = 0; i < ARGS.intensity; ++i) {
		double a = global.timer/sqrt(global.diff);
		cmplx dir = cdir(a);

		if (i > 90) {
			a *= -1;
		}

		PROJECTILE(
			.proto = pp_wave,
			.pos = e->pos,
			.color = (i & 3) ? RGB(1.0, 0.3, 0.3) : RGB(0.3, 0.3, 1.0),
			.move = move_linear(2 * dir)
		);

		if (global.diff > D_Normal && global.timer % 3 == 0) {

			PROJECTILE(
				.proto = pp_wave,
				.pos = e->pos,
				.color = !(i & 3) ? RGB(1.0, 0.3, 0.3) : RGB(0.3, 0.3, 1.0),
				.move = move_linear(-2 * dir)
			);
		}

		play_sfx("shot1");
		WAIT(ARGS.shot_interval);
	}

	WAIT(60);

}

TASK(little_fairy, { cmplx pos; cmplx target_pos; int shot_type; int side; }) {
	// contains stage3_slavefairy and stage3_slavefairy2
	Enemy *e = TASK_BIND(espawn_fairy_blue(ARGS.pos, ITEMS(
		.points = 3,
		.power = 1,
	)));

	// fade-in
	e->alpha = 0;

	e->move = move_towards(ARGS.target_pos, 0);

	int shot_interval = 1;
	int intensity = difficulty_value(30, 50, 70, 90);

	if(ARGS.shot_type){
		INVOKE_TASK(little_fairy_shot_ball, ENT_BOX(e), shot_interval, intensity);
	} else {
		INVOKE_TASK(little_fairy_shot_wave, ENT_BOX(e), shot_interval, intensity);
	}

	WAIT(120);

	// fly out
	cmplx exit_accel = 0.02 * I + (ARGS.side * 0.03);
	e->move.attraction = 0;
	e->move.acceleration = exit_accel;
	e->move.retention = 1;
}

TASK(little_fairy_line, { int count; }) {
	for(int i = 0; i < ARGS.count; ++i) {
		cmplx pos1 = VIEWPORT_W/2 + VIEWPORT_W/3 * rng_f32() + VIEWPORT_H/5*I;
		cmplx pos2 = VIEWPORT_W/2 + 100 * (i - ARGS.count/2) + VIEWPORT_H/3*I;
		INVOKE_TASK(little_fairy,
			.pos = pos1,
			.target_pos = pos2,
			.side = 2
		);
		WAIT(5);
	}
}

TASK(big_fairy_group, { cmplx pos; int shot_type; } ) {
	// big fairy in the middle does nothing
	// 8 fairies (2 pairs in 4 waves - bottom/top/bottom/top) spawn around her and open fire
	Enemy *e = TASK_BIND(espawn_huge_fairy(ARGS.pos, ITEMS(
		.points = 3,
		.power = 2,
	)));

	e->alpha = 0;
	WAIT(100);

	for(int x = 0; x < 2; ++x) {
		// type, big fairy pos, little fairy pos (relative), danmaku type, side of big fairy
		INVOKE_TASK(little_fairy,
			.pos = e->pos,
			.target_pos = e->pos + 70 + 50 * I,
			.shot_type = ARGS.shot_type,
			.side = 1
		);

		INVOKE_TASK(little_fairy,
			.pos = e->pos,
			.target_pos = e->pos - 70 + 50 * I,
			.shot_type = ARGS.shot_type,
			.side = -1
		);

		WAIT(100);

		INVOKE_TASK(little_fairy,
			.pos = e->pos,
			.target_pos = e->pos + 70 - 50 * I,
			.shot_type = ARGS.shot_type,
			.side = 1
		);

		INVOKE_TASK(little_fairy,
			.pos = e->pos,
			.target_pos = e->pos - 70 - 50 * I,
			.shot_type = ARGS.shot_type,
			.side = -1
		);

		WAIT(200);
	}


	e->move.attraction = 0;
	e->move.acceleration = 0.02 * I + 0;
	e->move.retention = 1;
}

TASK(burst_fairy, { cmplx pos; cmplx target_pos; } ) {
	Enemy *e = TASK_BIND(espawn_fairy_red(ARGS.pos, ITEMS(
		.points = 1,
		.power = 1,
	)));

	e->alpha = 0;

	e->move = move_towards(ARGS.target_pos, 0.08);
	WAIT(30);

	int difficulty = difficulty_value(4, 8, 12, 16);

	int count = 6 + difficulty;
	for(int p = 0; p < count; ++p) {
		cmplx dir = cdir(M_PI * 2 * p / count);
		PROJECTILE(
			.proto = pp_ball,
			.pos = ARGS.target_pos,
			.color = RGB(0.2, 0.1, 0.5),
			.move = move_asymptotic_simple(dir, 10 + difficulty)
		);
	}

	WAIT(60);
	play_sfx("shot_special1");
	INVOKE_TASK(common_charge, e->pos, RGBA(0.0, 0.5, 1.0, 0.5), 60, .sound = COMMON_CHARGE_SOUNDS);

	int intensity = difficulty_value(4, 6, 8, 10);
	for(int x = 0; x < intensity; ++x) {

		double phase = 0.1 * x;
		cmplx dir = cdir(M_PI * phase);

		for(int p = 0; p < intensity; ++p) {
			for(int i = -1; i < 2; i += 2) {
				PROJECTILE(
					.proto = pp_bullet,
					.pos = e->pos + dir * 10,
					.color = color_lerp(RGB(0.0, 0.0, 1.0), RGB(1.0, 0.0, 0.0), psin(M_PI * phase)),
					.move = move_asymptotic_simple(1.5 * dir * (1 + p / (intensity - 1.0)) * i, 3 * difficulty),
				);
			}
		}
		WAIT(2);
	}
	WAIT(5);

	cmplx exit_accel = 0.02 * I + 0.03;
	e->move.attraction = 0;
	e->move.acceleration = exit_accel;
	e->move.retention = 1;
}

TASK(burst_fairy_squad, { int count; int step; } ) {
	// fires a bunch of "lines" of bullets in a starburst pattern
	// does so after waiting for a short amount of time

	for(int i = 0; i < ARGS.count; ++i) {
		double span = 300 - 60 * (i/2);
		cmplx pos = VIEWPORT_W/2 + span * (-0.5 + (i & 1)) + (VIEWPORT_H/3 + 100 * (i / 2)) * I;
		// type, starting_position, target_position
		INVOKE_TASK(burst_fairy, VIEWPORT_W/2, pos);
		WAIT(ARGS.step);
	}
}

TASK(charge_fairy, { cmplx pos; cmplx target_pos; cmplx exit_dir; int charge_time; int move_first; }) {
	// charges up some danmaku and then "releases" them
	Enemy *e = TASK_BIND(espawn_fairy_red(ARGS.pos, ITEMS(
		.points = 1,
		.power = 4,
	)));

	e->alpha = 0;

	if(ARGS.move_first) {
		e->move = move_towards(ARGS.target_pos, 0.03);
		WAIT(100);
	}

	play_sfx("shot_special1");
	INVOKE_TASK(common_charge, e->pos, RGBA(0.0, 0.5, 1.0, 0.5), ARGS.charge_time, .sound = COMMON_CHARGE_SOUNDS);
	WAIT(ARGS.charge_time + 20);

	int intensity = difficulty_value(11, 15, 19, 23);
	int layers = difficulty_value(2, 3, 4, 5);
	int nproj = intensity * layers;

	DECLARE_ENT_ARRAY(Projectile, projs, nproj);

	for(int x = 0; x < intensity; ++x) {
		cmplx aim = cnormalize(global.plr.pos - e->pos);
		aim /= cabs(aim);
		cmplx aim_norm = aim * I;

		int i = x;

		for(int layer = 0; layer < layers; ++layer) {
			if(layer&1) {
				i = intensity - 1 - i;
			}

			double f = i / (intensity - 1.0);
			int w = 100 - 20 * layer;
			cmplx o = e->pos + w * psin(M_PI * f) * aim + aim_norm * w * 0.8 * (f - 0.5);
			cmplx paim = e->pos + (w+1) * aim - o;
			paim /= cabs(paim);

			cmplx v = paim * 0.2 * (6 + global.diff - layer);

			ENT_ARRAY_ADD(&projs, PROJECTILE(
				.proto = pp_wave,
				.pos = o,
				.color = color_lerp(RGB(0.0, 0.0, 1.0), RGB(1.0, 0.0, 0.0), f),
				.move = move_linear(v),
				.angle = carg(v),
				.flags = PFLAG_NOMOVE,
			));
		}
	}

	ENT_ARRAY_FOREACH(&projs, Projectile *p, {
		spawn_projectile_highlight_effect(p);
		p->flags &= ~PFLAG_NOMOVE;
		play_sfx("redirect");
		play_sfx("shot_special1");
	});

	WAIT(100);

	e->move = move_linear(ARGS.exit_dir);
	ENT_ARRAY_CLEAR(&projs);
}

TASK(charge_fairy_squad_1, { int count; int step; int charge_time; } ) {
	// squad of small fairies that fire tight groups of "charged shots"
	// and then fly off screen in random directions

	for(int x = 0; x < ARGS.count; ++x) {
		int i = x % 4;
		double span = 300 - 60 * (i/2);
		cmplx pos = VIEWPORT_W/2 + span * (-0.5 + (i & 1)) + (VIEWPORT_H/3 + 100*(i / 2)) * I;
		cmplx exitdir = pos - (VIEWPORT_W+VIEWPORT_H * I) / 2;
		exitdir /= cabs(exitdir);

		INVOKE_TASK(charge_fairy,
			.pos = pos,
			.target_pos = 0,
			.exit_dir = exitdir,
			.charge_time = ARGS.charge_time,
			.move_first = 0
		);

		WAIT(ARGS.step);
	}
}

TASK(charge_fairy_squad_2, { int count; cmplx start_pos; cmplx target_pos; cmplx exit_dir; int delay; int charge_time; } ) {
	for(int x = 0; x < ARGS.count; ++x) {
		INVOKE_TASK(charge_fairy, ARGS.start_pos, ARGS.target_pos, ARGS.exit_dir, ARGS.charge_time, 1);
		WAIT(ARGS.delay);
	}
}

TASK(corner_fairy, { cmplx pos; cmplx p1; cmplx p2; int type; } ) {
	Enemy *e = TASK_BIND(espawn_fairy_red(ARGS.pos, ITEMS(
		.power = 2,
	)));

	e->alpha = 0;

	INVOKE_TASK_DELAYED(240, destroy_enemy, ENT_BOX(e));

	e->move = move_towards(ARGS.p1, 0.02);
	WAIT(100);

	int intensity = difficulty_value(7, 14, 21, 28);

	for(int x = 0; x < 100; ++x) {
		int momentum = 140 + x;
		e->move.attraction_point = ARGS.p2;
		e->move.attraction = 0.025 * fmin((20 + x) / 42.0, 1);

		int d = 5;
		if(!(momentum % d)) {

			for(int i = 0; i < intensity; ++i) {
				float c = psin(momentum / 15.0);
				bool wave = global.diff > D_Easy && ARGS.type;

				PROJECTILE(
					.proto = wave ? pp_wave : pp_thickrice,
					.pos = e->pos,
					.color = ARGS.type ? RGB(0.5 - c*0.2, 0.3 + c*0.7, 1.0) : RGB(1.0 - c*0.5, 0.6, 0.5 + c*0.5),
					.move = move_asymptotic_simple(
						((1.8 - 0.4 * wave * !!ARGS.p2) * cdir((2 * i * M_PI / intensity) + carg((VIEWPORT_W + I * VIEWPORT_H)/2 - e->pos))),
						1.5
					)
				);
				play_sfx("shot1_loop");
				WAIT(1);
			}
		}
	}
}

TASK(corner_fairies, NO_ARGS) {
	double offs = -50;

	cmplx p1 = 0+0*I;
	cmplx p2 = VIEWPORT_W+0*I;
	cmplx p3 = VIEWPORT_W+VIEWPORT_H*I;
	cmplx p4 = 0+VIEWPORT_H*I;

	INVOKE_TASK(corner_fairy, p1, p1 - offs - offs*I, p2 + offs - offs*I);
	INVOKE_TASK(corner_fairy, p2, p2 + offs - offs*I, p3 + offs + offs*I);
	INVOKE_TASK(corner_fairy, p3, p3 + offs + offs*I, p4 - offs + offs*I);
	INVOKE_TASK(corner_fairy, p4, p4 - offs + offs*I, p1 - offs - offs*I);

	WAIT(90);

	if(global.diff > D_Normal) {
		INVOKE_TASK(corner_fairy, p1, p1 - offs*I, p2 + offs);
		INVOKE_TASK(corner_fairy, p2, p2 + offs, p3 + offs*I);
		INVOKE_TASK(corner_fairy, p3, p3 + offs*I, p4 - offs);
		INVOKE_TASK(corner_fairy, p4, p4 - offs, p1 + offs*I);
	}
}

// bosses

TASK_WITH_INTERFACE(midboss_intro, BossAttack) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);

	boss->move = move_towards(VIEWPORT_W/2 + 100.0*I, 0.04);
}

TASK_WITH_INTERFACE(midboss_outro, BossAttack) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);

	// TODO: this doesn't match the movement in the original
	// but I have no clue how to replicate it cleanly...
	// original:
	// boss->pos += pow(max(0, time)/30.0, 2) * cexp(I*(3*M_PI/2 + 0.5 * sin(time / 20.0)));
	boss->move = move_towards(VIEWPORT_W/2 - 200.0*I, 0.05);
}

TASK(spawn_midboss, NO_ARGS) {
	STAGE_BOOKMARK_DELAYED(120, midboss);

	Boss *boss = global.boss = stage3_spawn_scuttle(VIEWPORT_W/2 - 200.0*I);
	boss_add_attack_task(boss, AT_Move, "Introduction", 1, 0, TASK_INDIRECT(BossAttack, midboss_intro), NULL);
	boss_add_attack_task(boss, AT_Normal, "Lethal Bite", 11, 20000, TASK_INDIRECT(BossAttack, stage3_midboss_nonspell_1), NULL);
	boss_add_attack_from_info(boss, &stage3_spells.mid.deadly_dance, false);
	boss_add_attack_task(boss, AT_Move, "Runaway", 2, 1, TASK_INDIRECT(BossAttack, midboss_outro), NULL);
	boss->zoomcolor = *RGB(0.4, 0.1, 0.4);

	boss_engage(boss);
}

TASK(boss_appear, { BoxedBoss boss; }) {
	Boss *boss = TASK_BIND(ARGS.boss);
	boss->move = move_towards(VIEWPORT_W/2.0 + 100.0*I, 0.05);
}

TASK(spawn_boss, NO_ARGS) {
	STAGE_BOOKMARK_DELAYED(120, boss);

	Boss *boss = global.boss = stage3_spawn_wriggle(VIEWPORT_W/2 - 200.0*I);

	PlayerMode *pm = global.plr.mode;
	Stage3PreBossDialogEvents *e;
	INVOKE_TASK_INDIRECT(Stage3PreBossDialog, pm->dialog->Stage3PreBoss, &e);
	INVOKE_TASK_WHEN(&e->boss_appears, boss_appear, ENT_BOX(boss));
	INVOKE_TASK_WHEN(&e->music_changes, common_start_bgm, "stage3boss");
	WAIT_EVENT(&global.dialog->events.fadeout_began);

	boss_add_attack_task(boss, AT_Normal, "", 11, 35000, TASK_INDIRECT(BossAttack, stage3_boss_nonspell_1), NULL);
	boss_add_attack_from_info(boss, &stage3_spells.boss.moonlight_rocket, false);
	boss_add_attack_task(boss, AT_Normal, "", 40, 35000, TASK_INDIRECT(BossAttack, stage3_boss_nonspell_2), NULL);
	boss_add_attack_from_info(boss, &stage3_spells.boss.wriggle_night_ignite, false);
	boss_add_attack_task(boss, AT_Normal, "", 40, 35000, TASK_INDIRECT(BossAttack, stage3_boss_nonspell_3), NULL);
	boss_add_attack_from_info(boss, &stage3_spells.boss.firefly_storm, false);
	boss_add_attack_from_info(boss, &stage3_spells.extra.light_singularity, false);

	boss_engage(boss);
}

DEFINE_EXTERN_TASK(stage3_timeline) {
	stage_start_bgm("stage3");
	stage_set_voltage_thresholds(50, 125, 300, 600);

	// 14 swirls that die in an explosion after a second
	INVOKE_TASK_DELAYED(200, burst_swirls,
		.count = 14,
		.interval = 10,
		.shot_proj = pp_rice
	);

	// big fairy with 4-8 sub-fairies
	INVOKE_TASK_DELAYED(360, big_fairy_group,
		.pos = VIEWPORT_W/2 + (VIEWPORT_H/3)*I,
		.shot_type = 1
	);

	// line of swirls that fly in across the screen in a shifting arc
	INVOKE_TASK_DELAYED(600, side_swirls_procession,
		.interval = 20,
		.count = 6,
		.move = move_asymptotic(5*I, 5, 0.95),
		.start_pos = -20 + 20*I
	);

	INVOKE_TASK_DELAYED(800, side_swirls_procession,
		.interval = 20,
		.count = 6,
		.move = move_asymptotic(1*I, 5, 0.95),
		.start_pos = -20 + 20*I
	);

	INVOKE_TASK_DELAYED(820, side_swirls_procession,
		.interval = 20,
		.count = 6,
		.move = move_asymptotic(1*I, -5, 0.95),
		.start_pos = VIEWPORT_W+20 + 20*I
	);

	INVOKE_TASK_DELAYED(1030, burst_fairy_squad,
		.count = 4,
		.step = 20
	);

	INVOKE_TASK_DELAYED(1300, charge_fairy_squad_1,
		.count = 6,
		.step = 60,
		.charge_time = 30
	);

	INVOKE_TASK_DELAYED(1530, side_swirls_procession,
		.interval = 20,
		.count = 5,
		.move = move_linear(5*I),
		.start_pos = 20 - 20*I
	);

	INVOKE_TASK_DELAYED(1575, side_swirls_procession,
		.interval = 20,
		.count = 5,
		.move = move_linear(5*I),
		.start_pos = VIEWPORT_W-20 - 20*I
	);

	INVOKE_TASK_DELAYED(1600, big_fairy_group,
		.pos = VIEWPORT_W/2 + (VIEWPORT_H/3)*I,
		.shot_type = 1
	);

	INVOKE_TASK_DELAYED(1800, little_fairy_line,
		.count = 3
	);

	INVOKE_TASK_DELAYED(2000, charge_fairy_squad_2,
		.count = 3,
		.start_pos = -20 + (VIEWPORT_H+20)*I,
		.target_pos = 30 + VIEWPORT_H/2.0*I,
		.exit_dir = -1*I,
		.delay = 60,
		.charge_time = 30
	);

	INVOKE_TASK_DELAYED(2000, charge_fairy_squad_2,
		.count = 3,
		.start_pos = VIEWPORT_W+20 + (VIEWPORT_H+20)*I,
		.target_pos = VIEWPORT_W-30 + VIEWPORT_H/2.0*I,
		.exit_dir = -1*I,
		.delay = 60,
		.charge_time = 30
	);

	INVOKE_TASK_DELAYED(2400, corner_fairies);

	STAGE_BOOKMARK_DELAYED(2500, pre-midboss);

	INVOKE_TASK_DELAYED(2700, spawn_midboss);

	while(!global.boss) YIELD;
	WAIT_EVENT(&global.boss->events.defeated);

	WAIT(150);

	STAGE_BOOKMARK(post-midboss);

	INVOKE_TASK_DELAYED(100, side_swirls_procession,
		.interval = 20,
		.count = 10,
		.move = move_asymptotic(-25*I, 5, 0.95),
		.start_pos = -20 + (VIEWPORT_H-20)*I
	);

	INVOKE_TASK_DELAYED(105, side_swirls_procession,
		.interval = 20,
		.count = 10,
		.move = move_asymptotic(-25*I, -5, 0.95),
		.start_pos = VIEWPORT_W+20 + (VIEWPORT_H-20)*I
	);

	INVOKE_TASK_DELAYED(250, burst_swirls,
		.count = 14,
		.interval = 10,
		.shot_proj = pp_ball
	);

	INVOKE_TASK_DELAYED(400, big_fairy_group,
		.pos = VIEWPORT_W - VIEWPORT_W/3 + (VIEWPORT_H/5)*I,
		.shot_type = 2
	);

	INVOKE_TASK_DELAYED(500, side_swirls_procession,
		.interval = 20,
		.count = 10,
		.move = move_linear(-5*I),
		.start_pos = VIEWPORT_W-20 + (VIEWPORT_H+20)*I
	);

	INVOKE_TASK_DELAYED(1000, big_fairy_group,
		.pos = VIEWPORT_W/3 + (VIEWPORT_H/5)*I,
		.shot_type = 2
	);

	INVOKE_TASK_DELAYED(1200, charge_fairy_squad_1,
		.count = 4,
		.step = 60,
		.charge_time = 30
	);

	INVOKE_TASK_DELAYED(1500, burst_fairy_squad,
		.count = 4,
		.step = 70
	);

	INVOKE_TASK_DELAYED(1700, side_swirls_procession,
		.interval = 20,
		.count = 10,
		.move = move_linear(-5*I),
		.start_pos = 20 + (VIEWPORT_H+20)*I
	);

	INVOKE_TASK_DELAYED(2200, corner_fairies);

	INVOKE_TASK_DELAYED(2300, side_swirls_procession,
		.interval = 20,
		.count = 10,
		.move = move_linear(5*I),
		.start_pos = VIEWPORT_W-20 - 20.0*I
	);

	INVOKE_TASK_DELAYED(2600, side_swirls_procession,
		.interval = 20,
		.count = 10,
		.move = move_linear(5*I),
		.start_pos = 20 + -20.0*I
	);

	STAGE_BOOKMARK_DELAYED(2800, pre-boss);

	WAIT(3000);

	stage_unlock_bgm("stage3boss");

	INVOKE_TASK(spawn_boss);

	while(!global.boss) YIELD;
	WAIT_EVENT(&global.boss->events.defeated);

	WAIT(240);
	stage3_dialog_post_boss();
	WAIT_EVENT(&global.dialog->events.fadeout_began);

	WAIT(5);
	stage_finish(GAMEOVER_SCORESCREEN);
}
