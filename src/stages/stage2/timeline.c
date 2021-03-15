/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "timeline.h"
#include "wriggle.h"
#include "hina.h"
#include "nonspells/nonspells.h"
#include "stage2.h"
#include "background_anim.h"

#include "stage.h"
#include "global.h"
#include "common_tasks.h"
#include "enemy_classes.h"

TASK(spinshot_fairy_attack_spawn_projs, {
	BoxedEnemy e;
	BoxedProjectileArray *projs;
	cmplx *proj_origins;
	int spawn_period;
	cmplx initial_offset;
	const Color *color;
}) {
	Enemy *e = TASK_BIND(ARGS.e);
	int count = ARGS.projs->capacity;

	cmplx ofs = ARGS.initial_offset;
	cmplx r = cdir(2*M_PI/count);

	for(int i = 0; i < count; ++i) {
		play_sfx_loop("shot1_loop");

		cmplx o = e->pos;
		ARGS.proj_origins[i] = o;

		ENT_ARRAY_ADD(ARGS.projs, PROJECTILE(
			.proto = pp_wave,
			.color = ARGS.color,
			.pos = o,
			.move = move_towards(o + ofs, 0.1),
			.max_viewport_dist = 128,
		));

		ofs *= r;
		WAIT(ARGS.spawn_period);
	}
}

TASK(spinshot_fairy_attack, {
	BoxedEnemy e;
	int count;
	int spawn_period;
	int activate_time;
	cmplx initial_offset;
	cmplx activated_vel_multiplier;
	cmplx activated_accel_multiplier;
	cmplx activated_retention_multiplier;
	const Color *color;
}) {
	int count = ARGS.count;
	Color color = *ARGS.color;

	DECLARE_ENT_ARRAY(Projectile, projs, count);
	cmplx proj_origins[count];

	BoxedTask spawner_task = cotask_box(
		INVOKE_SUBTASK(spinshot_fairy_attack_spawn_projs,
			ARGS.e,
			&projs,
			proj_origins,
			ARGS.spawn_period,
			ARGS.initial_offset,
			&color
		)
	);

	cmplx ref_pos = NOT_NULL(ENT_UNBOX(ARGS.e))->pos;
	Enemy *e;

	for(int t = 0; t < ARGS.activate_time; ++t, YIELD) {
		int live_count = 0;
		e = ENT_UNBOX(ARGS.e);

		if(e) {
			ref_pos = e->pos;
		}

		ENT_ARRAY_FOREACH_COUNTER(&projs, int i, Projectile *p, {
			cmplx ofs = ref_pos - proj_origins[i];
			proj_origins[i] += ofs;
			p->pos += ofs;
 			p->move.attraction_point += ofs;
			++live_count;
		});

 		if(live_count == 0) {
			CoTask *spawner_task_unboxed = cotask_unbox(spawner_task);
			if(!spawner_task_unboxed || cotask_status(spawner_task_unboxed) == CO_STATUS_DEAD) {
				log_debug("spawner is dead and no live projectiles, quitting");
				return;
			}
 		}
	}

	int live_count = 0;

	ENT_ARRAY_FOREACH(&projs, Projectile *p, {
		spawn_projectile_highlight_effect(p);
		cmplx dir = cdir(p->angle);
		p->move = move_accelerated(dir * ARGS.activated_vel_multiplier, dir * ARGS.activated_accel_multiplier);
		p->move.retention *= ARGS.activated_retention_multiplier;
		++live_count;
	});

	if(live_count == 0) {
		return;
	}

	play_sfx("redirect");

	for(;live_count > 0; YIELD) {
		live_count = 0;
		ENT_ARRAY_FOREACH(&projs, Projectile *p, {
			if(capproach_asymptotic_p(&p->move.retention, 1, 0.02, 1e-3) != 1) {
				++live_count;
			}
		});
	}
}

TASK(spinshot_fairy, { cmplx pos; MoveParams move_enter; MoveParams move_exit; }) {
	Enemy *e = TASK_BIND(espawn_big_fairy(ARGS.pos, ITEMS(.points = 5, .power = 4)));
	e->move = ARGS.move_enter;

	int count = difficulty_value(8, 10, 12, 14);
	int charge_time = difficulty_value(100, 80, 60, 60);
	int spawn_period = charge_time / count + (charge_time % count != 0);
	charge_time = count * spawn_period;

	int activate_delay = 42;
	charge_time += activate_delay;

	int waves = 10;
	int wave_period = 1;

	INVOKE_SUBTASK(common_charge,
		.time = charge_time + waves * wave_period,
		.anchor = &e->pos,
		.color = RGBA(1.0, 0.1, 0.1, 0.0),
		.sound = COMMON_CHARGE_SOUNDS
	);

	INVOKE_SUBTASK_DELAYED(5, common_charge,
		.time = charge_time + waves * wave_period,
		.anchor = &e->pos,
		.color = RGBA(0.1, 0.1, 1.0, 0.0)
	);

	cmplx dir = rng_dir();

	for(int i = 0; i < waves; ++i, WAIT(wave_period)) {
		play_sfx_loop("shot1_loop");

		INVOKE_TASK(spinshot_fairy_attack,
			.e = ENT_BOX(e),
			.count = count,
			.spawn_period = spawn_period,
			.activate_time = charge_time + 2 * (10 - i) + 14,
			.initial_offset = (24 + 10 * i) * dir,
			.activated_vel_multiplier = -3,
			.activated_accel_multiplier = 0.05*I,
			.activated_retention_multiplier = cdir(0.1),
			.color = RGBA(1.0, 0, 0, 0.5)
		);

		INVOKE_TASK(spinshot_fairy_attack,
			.e = ENT_BOX(e),
			.count = count,
			.spawn_period = spawn_period,
			.activate_time = charge_time + 2 * (10 - i),
			.initial_offset = (24 + 10 * i) / dir,
			.activated_vel_multiplier = 3,
			.activated_accel_multiplier = 0.05/I,
			.activated_retention_multiplier = cdir(-0.1),
			.color = RGBA(0.0, 0, 1.0, 0.5)
		);

		dir *= cdir(0.3);
	}

	WAIT(charge_time + 120);

	e->move = ARGS.move_exit;

	STALL;
}

TASK(starcaller_fairy, { cmplx pos; MoveParams move_enter; MoveParams move_exit; }) {
	Enemy *e = TASK_BIND(espawn_huge_fairy(ARGS.pos, ITEMS(.points = 5, .power = 5)));
	e->move = ARGS.move_enter;

	int charge_time = difficulty_value(120, 80, 60, 60);
	common_charge(charge_time, &e->pos, 0, RGBA(0.5, 0.2, 1.0, 0.0));

	int step = difficulty_value(60, 40, 40, 30);
	int interstep = difficulty_value(25, 20, 20, 15);
	int arcshots = difficulty_value(10, 10, 12, 14);
	int num_bursts = difficulty_value(4, 8, 12, 16);
	int endpoints = 5;
	int totalshots = endpoints * (1 + arcshots);

	cmplx rotate_per_shot = cdir(2*M_PI/totalshots);
	cmplx rotate_per_burst = cdir(M_PI/endpoints);
	cmplx dir = rng_dir();

	real boostfactor = 2;
	real speed = difficulty_value(2.0, 2.0, 2.5, 2.5);
	real initial_offset = 8;

	for(int burst = 0; burst < num_bursts; ++burst) {
		play_sfx("shot_special1");
		bool inward = burst & 1;

		for(int i = 0; i < endpoints; ++i) {
			MoveParams move_ball = move_asymptotic_simple(speed * dir, inward ? 0 : boostfactor);

			PROJECTILE(
				.proto = pp_bigball,
				.pos = e->pos + initial_offset * move_ball.velocity,
				.color = RGB(0.3, 0.0, 0.6),
				.flags = PFLAG_MANUALANGLE,
				.move = move_ball
			);

			dir *= rotate_per_shot;

			for(int j = 0; j < arcshots; ++j) {
				real boost = j / (arcshots - 1.0);
				boost -= boost * boost;
				boost = 1 - 4 * boost;
				boost *= boostfactor;

				MoveParams move_rice = move_asymptotic_simple(speed * dir, inward ? boostfactor - boost : boost);

				PROJECTILE(
					.proto = pp_rice,
					.pos = e->pos + initial_offset * move_rice.velocity,
					.color = RGB(0.6 + boost / boostfactor, 0.0, 0.3),
					.move = move_rice,
				);

				dir *= rotate_per_shot;
			}
		}

		dir *= rotate_per_burst;
		WAIT(inward ? step : interstep);
	}

	WAIT(60);

	e->move = ARGS.move_exit;
}

TASK(redwall_fairy, {
	cmplx pos;
	real shot_x_dir;   // FIXME: bad name, but the shot logic is really confusing so not sure what to call this
	MoveParams move;
}) {
	Enemy *e = TASK_BIND(espawn_fairy_red(ARGS.pos, ITEMS(.power = 2)));
	e->move = ARGS.move;

	// likewise, bad name
	cmplx shot_dir = ARGS.shot_x_dir + 3*I;

	WAIT(30);

	int duration = difficulty_value(42, 60, 65, 70);
	int step = difficulty_value(5, 4, 4, 3);
	real pspeed = difficulty_value(2, 2, 2.2, 2.7);

	for(int t = 0; t < duration; t += WAIT(step)) {
		play_sfx("shot3");

		PROJECTILE(
			.proto = pp_ball,
			.pos = e->pos,
			.color = RGB(0.9, 0.0, 0.3),
			.move = move_linear(pspeed * (conj(e->pos - VIEWPORT_W/2) / 100 + shot_dir)),
		);
	}
}

TASK(aimshot_fairy, { BoxedEnemy e; MoveParams move_enter; MoveParams move_exit; }) {
	Enemy *e = TASK_BIND(ARGS.e);
	e->move = ARGS.move_enter;

	WAIT(42);
	play_sfx("shot1");

	cmplx aim = cnormalize(global.plr.pos - e->pos);
	int num = difficulty_value(1, 1, 3, 5);
	real speed1 = difficulty_value(3, 4, 5, 5);
	real speed2 = difficulty_value(3, 3, 4, 4);

	for(int i = 0; i < num; ++i) {
		cmplx dir = aim;

		if(num > 1) {
			dir *= cdir((i / (num - 1.0) - 0.5) * M_TAU/16);
		}

		PROJECTILE(
			.proto = pp_plainball,
			.pos = e->pos,
			.color = RGB(0.6, 0.0, 0.8),
			.move = move_asymptotic_simple(speed1 * dir, -1),
		);

		if(global.diff > D_Easy) {
			PROJECTILE(
				.proto = pp_plainball,
				.pos = e->pos,
				.color = RGB(0.2, 0.0, 0.1),
				.move = move_asymptotic_simple(speed2 * dir, 2),
			);
		}
	}

	WAIT(60);

	e->move = ARGS.move_exit;
}

TASK(turning_fairy, {
	BoxedEnemy e;
	cmplx vel;
	real turn_angle;
	int turn_delay;
	int turn_duration;
}) {
	Enemy *e = TASK_BIND(ARGS.e);
	e->move = move_linear(ARGS.vel);
	INVOKE_SUBTASK(common_move_turn,
		.move_params = &e->move,
		.turn_angle = ARGS.turn_angle,
		.turn_delay = ARGS.turn_delay,
		.turn_duration = ARGS.turn_duration
	);

	int period = difficulty_value(70, 50, 24, 16);
	WAIT(7 + period/2);

	for(int t = 0;; t += WAIT(period)) {
		play_sfx_ex("shot1", 5, false);
		cmplx shot_normal = 3 * cnormalize(e->move.velocity);
		cmplx shot_org = e->pos + e->move.velocity * 4;

		PROJECTILE(
			.proto = pp_bullet,
			.pos = shot_org,
			.color = RGB(0.9, 0.0, 0.9),
			.move = move_asymptotic_simple(shot_normal*I, 3),
		);

		PROJECTILE(
			.proto = pp_bullet,
			.pos = shot_org,
			.color = RGB(0.9, 0.0, 0.9),
			.move = move_asymptotic_simple(shot_normal/I, 3),
		);
	}

	STALL;
}

TASK(turning_fairy_red, {
	cmplx pos;
	cmplx vel;
	real turn_angle;
	int turn_delay;
	int turn_duration;
}) {
	INVOKE_TASK(turning_fairy,
		.e = espawn_fairy_red_box(ARGS.pos, ITEMS(.power = 1)),
		.vel = ARGS.vel,
		.turn_angle = ARGS.turn_angle,
		.turn_delay = ARGS.turn_delay,
		.turn_duration = ARGS.turn_duration
	);
}

TASK(turning_fairy_blue, {
	cmplx pos;
	cmplx vel;
	real turn_angle;
	int turn_delay;
	int turn_duration;
}) {
	INVOKE_TASK(turning_fairy,
		.e = espawn_fairy_blue_box(ARGS.pos, ITEMS(.points = 1)),
		.vel = ARGS.vel,
		.turn_angle = ARGS.turn_angle,
		.turn_delay = ARGS.turn_delay,
		.turn_duration = ARGS.turn_duration
	);
}

TASK(flea_swirl, {
	cmplx pos;
	cmplx vel;
	real turn_angle;
	int turn_delay;
	int turn_duration;
}) {
	Enemy *e = TASK_BIND(espawn_swirl(ARGS.pos, ITEMS(.points = 2)));
	e->move = move_linear(ARGS.vel);
	INVOKE_SUBTASK(common_move_turn,
		.move_params = &e->move,
		.turn_angle = ARGS.turn_angle,
		.turn_delay = ARGS.turn_delay,
		.turn_duration = ARGS.turn_duration
	);

	int base_period = difficulty_value(27, 24, 21, 18);
	int duration = 400;

	WAIT(10 + base_period);

	for(int t = 0; t < duration; t += WAIT(base_period - t/70)) {
		play_sfx("shot2");
		PROJECTILE(
			.proto = pp_flea,
			.pos = e->pos,
			.color = RGB(0.3, 0.2, 1),
			.move = move_asymptotic_simple(1.5 * I * cdir(rng_sreal()*M_PI/17), 1.5),
		);
	}
}

static void stage2_dialog_post_boss(void) {
	PlayerMode *pm = global.plr.mode;
	INVOKE_TASK_INDIRECT(Stage2PostBossDialog, pm->dialog->Stage2PostBoss);
}

static void wriggle_ani_flyin(Boss *w) {
	aniplayer_queue(&w->ani,"midbossflyintro",1);
	aniplayer_queue(&w->ani,"main",0);
}

static void wriggle_intro_stage2(Boss *w, int t) {
	if(t < 0)
		return;
	t = smoothstep(0.0, 1.0, t / 240.0) * 240;
	w->pos = CMPLX(VIEWPORT_W/2, 100.0) + (1.0 - t / 240.0) * (300 * cexp(I * (3 - t * 0.04)) - 128);
}

static void wiggle_mid_flee(Boss *w, int t) {
	if(t == 0)
		aniplayer_queue(&w->ani, "fly", 0);
	if(t >= 0) {
		GO_TO(w, VIEWPORT_W/2 - 3.0 * t - 300 * I, 0.01)
	}
}

TASK(spawn_midboss, NO_ARGS) {
	STAGE_BOOKMARK(midboss);

	Boss *boss = global.boss = stage2_spawn_wriggle(VIEWPORT_W + 150 - 30.0*I);
	wriggle_ani_flyin(boss);

	boss_add_attack(boss, AT_Move, "Introduction", 4, 0, wriggle_intro_stage2, NULL);
	boss_add_attack_task(boss, AT_Normal, "", 20, 26000, TASK_INDIRECT(BossAttack, stage2_midboss_nonspell_1), NULL);
	boss_add_attack(boss, AT_Move, "Flee", 5, 0, wiggle_mid_flee, NULL);

	boss_engage(boss);
}

TASK(redwall_side_fairies, { int num; }) {
	for(int i = 0; i < ARGS.num; ++i) {
		real xdir = 1 - 2 * (i % 2);

		INVOKE_TASK(redwall_fairy,
			.pos = VIEWPORT_W * (i % 2),
			.shot_x_dir = xdir,
			.move = move_asymptotic_halflife(2 * xdir + I, 3.65 * xdir + 5*I, 50)
		);

		WAIT(50);
	}
}

TASK(redwall_side_fairies_2, { int num; }) {
	for(int i = 0; i < ARGS.num; ++i) {
		real xdir = 1 - 2 * (i % 2);

		INVOKE_TASK(redwall_fairy,
			.pos = VIEWPORT_H*0.5*I,
			.shot_x_dir = 1,
			.move = move_asymptotic_halflife(2 - 0.5*I, 3.65 - 2.5*I, 50)
		);

		INVOKE_TASK_DELAYED(20, redwall_fairy,
			.pos = VIEWPORT_W + VIEWPORT_H*0.5*I,
			.shot_x_dir = -xdir,
			.move = move_asymptotic_halflife(-2 - 0.5*I, -3.65 - 2.5*I, 50)
		);

		WAIT(50);
	}
}

static void aimshot_fairies(EnemySpawner spawner, const ItemCounts *items) {
	int num = difficulty_value(6, 7, 8, 8);
	int period = difficulty_value(20, 18, 15, 15);

	for(int i = 0; i < num; ++i) {
		cmplx pos = VIEWPORT_W/2 + 42 * (i - num/2.0) - 20.0*I;

		INVOKE_TASK(aimshot_fairy,
			.e = ENT_BOX(spawner(pos, items)),
			.move_enter = move_towards(pos + 180 * I, 0.03),
			.move_exit = move_accelerated(0, -0.15*I)
		);

		WAIT(period);
	}

}

TASK(aimshot_fairies_blue, NO_ARGS) {
	aimshot_fairies(espawn_fairy_blue, ITEMS(.points = 2));
}

TASK(aimshot_fairies_red, NO_ARGS) {
	aimshot_fairies(espawn_fairy_red, ITEMS(.power = 2));
}

DEFINE_TASK_INTERFACE(turning_fairies, {
	int num;
	int spawn_period;
	cmplx pos;
	cmplx vel;
	real turn_angle;
	int turn_delay;
	int turn_duration;
});

static void turning_fairies(
	EnemySpawner spawner,
	const ItemCounts *items,
	const TASK_IFACE_ARGS_TYPE(turning_fairies) *args
) {
	int num = args->num;
	int delay = args->spawn_period;

	for(int i = 0; i < num; ++i, WAIT(delay)) {
		INVOKE_TASK(turning_fairy,
			.e = ENT_BOX(spawner(args->pos, items)),
			.vel = args->vel,
			.turn_angle = args->turn_angle,
			.turn_delay = args->turn_delay,
			.turn_duration = args->turn_duration
		);
	}
}

TASK_WITH_INTERFACE(turning_fairies_red, turning_fairies) {
	turning_fairies(espawn_fairy_red, ITEMS(.power = 1), &ARGS);
}

TASK_WITH_INTERFACE(turning_fairies_blue, turning_fairies) {
	turning_fairies(espawn_fairy_blue, ITEMS(.points = 1), &ARGS);
}

TASK(flea_swirls, {
	int num;
	cmplx pos;
	cmplx vel;
	real turn_angle;
	int turn_delay;
	int turn_duration;
}) {
	int num = ARGS.num;

	for(int i = 0; i < num; ++i, WAIT(30)) {
		INVOKE_TASK(flea_swirl,
			.pos = ARGS.pos,
			.vel = ARGS.vel,
			.turn_angle = ARGS.turn_angle,
			.turn_delay = ARGS.turn_delay,
			.turn_duration = ARGS.turn_duration
		);
	}
}

TASK(boss_appear, { BoxedBoss boss; }) {
	Boss *boss = NOT_NULL(ENT_UNBOX(ARGS.boss));
	boss->move = move_towards(VIEWPORT_W/2 + 100.0*I, 0.05);

	aniplayer_queue(&boss->ani, "guruguru", 2);
	aniplayer_queue(&boss->ani, "main", 0);
}

TASK(spawn_boss, NO_ARGS) {
	STAGE_BOOKMARK(boss);

	Boss *boss = global.boss = stage2_spawn_hina(VIEWPORT_W + 180 + 100.0*I);

	PlayerMode *pm = global.plr.mode;
	Stage2PreBossDialogEvents *e;
	INVOKE_TASK_INDIRECT(Stage2PreBossDialog, pm->dialog->Stage2PreBoss, &e);
	INVOKE_TASK_WHEN(&e->boss_appears, boss_appear, ENT_BOX(boss));
	INVOKE_TASK_WHEN(&e->music_changes, common_start_bgm, "stage2boss");
	WAIT_EVENT(&global.dialog->events.fadeout_began);

	boss_add_attack_task(boss, AT_Normal, "Cards1", 60, 30000, TASK_INDIRECT(BossAttack, stage2_boss_nonspell_1), NULL);
	boss_add_attack_from_info(boss, &stage2_spells.boss.amulet_of_harm, false);
	boss_add_attack_task(boss, AT_Normal, "Cards2", 60, 20000, TASK_INDIRECT(BossAttack, stage2_boss_nonspell_2), NULL);
	boss_add_attack_from_info(boss, &stage2_spells.boss.bad_pick, false);
	boss_add_attack_task(boss, AT_Normal, "Cards3", 60, 45000, TASK_INDIRECT(BossAttack, stage2_boss_nonspell_3), NULL);
	boss_add_attack_from_info(boss, &stage2_spells.boss.wheel_of_fortune, false);
	boss_add_attack_from_info(boss, &stage2_spells.extra.monty_hall_danmaku, false);

	boss_engage(boss);
}

DEFINE_EXTERN_TASK(stage2_timeline) {
	YIELD;

	STAGE_BOOKMARK_DELAYED(300, init);

	INVOKE_TASK_DELAYED(300, starcaller_fairy,
		.pos = VIEWPORT_W/2 - 10.0*I,
		.move_enter = move_towards(VIEWPORT_W/2 + 150.0*I, 0.04),
		.move_exit = move_asymptotic_halflife(0, 2*I, 60)
	);

	INVOKE_TASK_DELAYED(difficulty_value(600, 550, 500, 450), redwall_side_fairies,
		.num = difficulty_value(4, 5, 6, 7)
	);

	INVOKE_TASK_DELAYED(750, aimshot_fairies_blue);

	INVOKE_TASK_DELAYED(825, aimshot_fairies_red);

	INVOKE_TASK_DELAYED(900, turning_fairies_red,
		.num = 12,
		.spawn_period = 20,
		.pos = VIEWPORT_W-80 + (VIEWPORT_H+20)*I,
		.vel = 3 / I,
		.turn_angle = -M_PI/2,
		.turn_delay = 90,
		.turn_duration = 80
	);

	INVOKE_TASK_DELAYED(1120, turning_fairies_blue,
		.num = 13,
		.spawn_period = 20,
		.pos = 200 - 20.0*I,
		.vel = 3 * I,
		.turn_angle = -1.5 * M_PI,
		.turn_delay = 90,
		.turn_duration = 120
	);

	INVOKE_TASK_DELAYED(1300, starcaller_fairy,
		.pos = 150 - 10.0*I,
		.move_enter = move_towards(150 + 100.0*I, 0.04),
		.move_exit = move_asymptotic_halflife(0, 2*I, 60)
	);

	INVOKE_TASK_DELAYED(1500, starcaller_fairy,
		.pos = VIEWPORT_W - 150 - 10.0*I,
		.move_enter = move_towards(VIEWPORT_W - 150 + 160.0*I, 0.04),
		.move_exit = move_asymptotic_halflife(0, 2*I, 60)
	);

	INVOKE_TASK_DELAYED(1600, turning_fairies_blue,
		.num = 13,
		.spawn_period = 20,
		.pos = 200 - 20.0*I,
		.vel = 3 * I,
		.turn_angle = 1.5 * M_PI,
		.turn_delay = 90,
		.turn_duration = 120
	);

	INVOKE_TASK_DELAYED(1700, flea_swirls,
		.num = 10,
		.pos = -15 + 30*I,
		.vel = 2,
		.turn_angle = M_PI,
		.turn_delay = (VIEWPORT_W - 96)/4,
		.turn_duration = 60
	);

	INVOKE_TASK_DELAYED(1850, flea_swirls,
		.num = 10,
		.pos = VIEWPORT_W + 15 + 30*I,
		.vel = -2,
		.turn_angle = -M_PI,
		.turn_delay = (VIEWPORT_W - 96)/4,
		.turn_duration = 60
	);

	STAGE_BOOKMARK_DELAYED(1950, post-flea);

	INVOKE_TASK_DELAYED(1950, turning_fairies_red,
		.num = 9,
		.spawn_period = 60,
		.pos = VIEWPORT_W-40 + (VIEWPORT_H+20)*I,
		.vel = 3 / I,
		.turn_angle = -M_PI/2,
		.turn_delay = 80,
		.turn_duration = 90
	);

	INVOKE_TASK_DELAYED(1950, turning_fairies_blue,
		.num = 9,
		.spawn_period = 60,
		.pos = 40 + (VIEWPORT_H+20)*I,
		.vel = 3 / I,
		.turn_angle = M_PI/2,
		.turn_delay = 80,
		.turn_duration = 90
	);

	INVOKE_TASK_DELAYED(2300, redwall_side_fairies,
		.num = 7
	);

	WAIT(2700);
	INVOKE_TASK(spawn_midboss);
	int midboss_time = WAIT_EVENT(&global.boss->events.defeated).frames;
	int filler_time = 1900;
	int time_ofs = 0 - midboss_time;

	STAGE_BOOKMARK(post-midboss);
	log_debug("midboss time: %i", midboss_time);

	filler_time -= WAIT(300 - midboss_time);

	for(int t = 0; t < 1400; t += 60) {
		INVOKE_TASK_DELAYED(t + time_ofs, turning_fairy_blue,
			.pos = VIEWPORT_W-60 + (VIEWPORT_H+20)*I,
			.vel = 3 / I,
			.turn_angle = -M_PI/2,
			.turn_delay = 80,
			.turn_duration = 120
		);

		INVOKE_TASK_DELAYED(t + time_ofs, turning_fairy_red,
			.pos = 60 + (VIEWPORT_H+20)*I,
			.vel = 3 / I,
			.turn_angle = M_PI/2,
			.turn_delay = 80,
			.turn_duration = 120
		);
	}

	STAGE_BOOKMARK_DELAYED(1100 + time_ofs, post-midboss-ideal);

	INVOKE_TASK_DELAYED(1360 + time_ofs, spinshot_fairy,
		.pos = VIEWPORT_W/2,
		.move_enter = move_towards(VIEWPORT_W/2+VIEWPORT_H/3*I, 0.02),
		.move_exit = move_accelerated(0, 0.1*I)
	);

	for(int i = 0; i < 10; ++i) {
		INVOKE_TASK_DELAYED(1560 + time_ofs + i * 30, flea_swirl,
			.pos = -15 + 30*I,
			.vel = 2,
			.turn_angle = M_PI,
			.turn_delay = (VIEWPORT_W - 96)/4,
			.turn_duration = 60
		);
	}

	INVOKE_TASK_DELAYED(1660 + time_ofs, starcaller_fairy,
		.pos = VIEWPORT_W - 150 - 10.0*I,
		.move_enter = move_towards(VIEWPORT_W - 150 + 160.0*I, 0.04),
		.move_exit = move_asymptotic_halflife(0, 2*I, 60)
	);

	WAIT(filler_time - midboss_time);
	STAGE_BOOKMARK(post-midboss-filler);

	INVOKE_TASK(aimshot_fairies_red);

	INVOKE_TASK_DELAYED(150, redwall_side_fairies_2,
		.num = 5
	);

	INVOKE_TASK_DELAYED(300, aimshot_fairies_blue);

	STAGE_BOOKMARK_DELAYED(400, twin-spinshots);

	if(global.diff > D_Normal) {
		INVOKE_TASK_DELAYED(420, spinshot_fairy,
			.pos = 0,
			.move_enter = move_towards(VIEWPORT_W/3+VIEWPORT_H/3*I, 0.02),
			.move_exit = move_accelerated(0, 0.1*I)
		);

		INVOKE_TASK_DELAYED(480, spinshot_fairy,
			.pos = VIEWPORT_W,
			.move_enter = move_towards(2*VIEWPORT_W/3+VIEWPORT_H/3*I, 0.02),
			.move_exit = move_accelerated(0, 0.1*I)
		);
	} else {
		INVOKE_TASK_DELAYED(420, spinshot_fairy,
			.pos = VIEWPORT_W/2,
			.move_enter = move_towards(VIEWPORT_W/2+VIEWPORT_H/3*I, 0.02),
			.move_exit = move_accelerated(0, 0.1*I)
		);
	}

	STAGE_BOOKMARK_DELAYED(720, pre-boss-spam);

	for(int i = 0; i < 4; ++i) {
		if(i & 1) {
			INVOKE_TASK_DELAYED(720 + 60 * i, aimshot_fairies_blue);
		} else {
			INVOKE_TASK_DELAYED(720 + 60 * i, aimshot_fairies_red);
		}
	}

	WAIT(860);
	stage2_bg_engage_hina_mode();
	WAIT(300);
	stage_clear_hazards(CLEAR_HAZARDS_ALL);
	INVOKE_TASK(spawn_boss);
	WAIT_EVENT(&global.boss->events.defeated);

	stage_unlock_bgm("stage2boss");

	WAIT(240);
	stage2_dialog_post_boss();
	WAIT_EVENT(&global.dialog->events.fadeout_began);

	WAIT(5);
	stage_finish(GAMEOVER_SCORESCREEN);
}
