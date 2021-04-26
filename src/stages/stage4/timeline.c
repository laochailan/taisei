/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "stage4.h"
#include "timeline.h"
#include "draw.h"
#include "background_anim.h"
#include "nonspells/nonspells.h"
#include "kurumi.h"

#include "global.h"
#include "stage.h"
#include "enemy.h"
#include "enemy_classes.h"
#include "laser.h"
#include "common_tasks.h"
#include "../stage6/elly.h"

/*
TASK(boss_appear_stub, NO_ARGS) {
	log_warn("FIXME");
}

static void stage4_dialog_pre_boss(void) {
	PlayerMode *pm = global.plr.mode;
	Stage4PreBossDialogEvents *e;
	INVOKE_TASK_INDIRECT(Stage4PreBossDialog, pm->dialog->Stage4PreBoss, &e);
	INVOKE_TASK_WHEN(&e->boss_appears, boss_appear_stub);
	INVOKE_TASK_WHEN(&e->music_changes, common_start_bgm, "stage4boss");
}

static void stage4_dialog_post_boss(void) {
	PlayerMode *pm = global.plr.mode;
	INVOKE_TASK_INDIRECT(Stage4PostBossDialog, pm->dialog->Stage4PostBoss);
}


static void kurumi_intro(Boss *b, int t) {
	GO_TO(b, BOSS_DEFAULT_GO_POS, 0.03);
}

static void kurumi_outro(Boss *b, int time) {
	b->pos += -5-I;
}

static Boss *create_kurumi_mid(void) {
	Boss *b = stage4_spawn_kurumi(VIEWPORT_W/2-400.0*I);

	boss_add_attack(b, AT_Move, "Introduction", 2, 0, kurumi_intro, NULL);
	boss_add_attack_from_info(b, &stage4_spells.mid.gate_of_walachia, false);
	if(global.diff < D_Hard) {
		boss_add_attack_from_info(b, &stage4_spells.mid.dry_fountain, false);
	} else {
		boss_add_attack_from_info(b, &stage4_spells.mid.red_spike, false);
	}
	boss_add_attack(b, AT_Move, "Outro", 2, 1, kurumi_outro, NULL);
	boss_start_attack(b, b->attacks);
	return b;
}

static void kurumi_boss_intro(Boss *b, int t) {
	TIMER(&t);
	GO_TO(b, BOSS_DEFAULT_GO_POS, 0.015);

	AT(120)
		stage4_dialog_pre_boss();
}

static Boss *create_kurumi(void) {
	Boss *b = stage4_spawn_kurumi(-400.0*I);

	boss_add_attack(b, AT_Move, "Introduction", 4, 0, kurumi_boss_intro, NULL);
	boss_add_attack(b, AT_Normal, "Sin Breaker", 25, 33000, kurumi_sbreaker, NULL);
	if(global.diff < D_Hard) {
		boss_add_attack_from_info(b, &stage4_spells.boss.animate_wall, false);
	} else {
		boss_add_attack_from_info(b, &stage4_spells.boss.demon_wall, false);
	}
	boss_add_attack(b, AT_Normal, "Cold Breaker", 25, 36000, kurumi_breaker, NULL);
	boss_add_attack_from_info(b, &stage4_spells.boss.bloody_danmaku, false);
	boss_add_attack_from_info(b, &stage4_spells.boss.blow_the_walls, false);
	boss_add_attack_from_info(b, &stage4_spells.extra.vlads_army, false);
	boss_start_attack(b, b->attacks);

	return b;
}

*/

TASK(splasher_fairy, { cmplx pos; int direction; }) {
	Enemy *e = TASK_BIND(espawn_big_fairy(ARGS.pos, ITEMS(.points = 3, .power = 1, .bomb = 1)));

	cmplx move = 3 * ARGS.direction - 4.0 * I;
	e->move = move_linear(move);
	WAIT(25);
	e->move.velocity = 0; // stop

	WAIT(50);
	for(int time = 0; time < 150; time += WAIT(5)) {
		// "shower" effect
		RNG_ARRAY(rand, 3);
		PROJECTILE(
			.proto = rng_chance(0.5) ? pp_rice : pp_thickrice,
			.pos = e->pos,
			.color = RGB(0.8, 0.3 - 0.1 * vrng_real(rand[0]), 0.5),
            .move = move_accelerated(move / 2 + (1 - 2 * vrng_real(rand[1])) + (1 - 2 * vrng_real(rand[2])) * I, 0.02 * I),
		);
		play_sfx("shot1_loop");
	}

	WAIT(60);
	e->move = move_linear(-ARGS.direction);
}

TASK(fodder_fairy, { cmplx pos; MoveParams move; }) {
	Enemy *e = TASK_BIND(espawn_fairy_red(ARGS.pos, ITEMS(.power = 1)));
	e->move = ARGS.move;

	WAIT(10);
	for(int i = 0; i < 2; i++, WAIT(120)) {

		cmplx fairy_halfsize = 21 * (1 + I);

		if(!rect_rect_intersect(
			(Rect) { e->pos - fairy_halfsize, e->pos + fairy_halfsize },
			(Rect) { 0, CMPLX(VIEWPORT_W, VIEWPORT_H) },
			true, true)
		) {
			continue;
		}

		play_sfx_ex("shot3", 5, false);
		cmplx aim = cnormalize(global.plr.pos - e->pos);

		real speed = 3;
		real boost_factor = 1.2;
		real boost_base = 1;
		int chain_len = difficulty_value(3, 4, 5, 6);

		PROJECTILE(
			.proto = pp_wave,
			.pos = e->pos,
			.color = RGB(1, 0.3, 0.5),
			.move = move_asymptotic_simple(speed * aim, (chain_len + 1) * boost_factor),
			.max_viewport_dist = 32,
		);

		for(int j = chain_len; j; --j) {
			PROJECTILE(
				.proto = pp_crystal,
				.pos = e->pos,
				.color = RGB(j / (float)(chain_len+1), 0.3, 0.5),
				.move = move_asymptotic_simple(speed * aim, boost_base + j * boost_factor),
				.max_viewport_dist = 32,
			);
		}

		PROJECTILE(
			.proto = pp_ball,
			.pos = e->pos,
			.color = RGB(0, 0.3, 0.5),
			.move = move_asymptotic_simple(speed * aim, boost_base),
			.max_viewport_dist = 32,
		);
	}
}

TASK(partcircle_fairy, { cmplx pos; MoveParams move; }) {
	Enemy *e = TASK_BIND(espawn_fairy_blue(ARGS.pos, ITEMS(.points = 3)));

	e->move = ARGS.move;

	// TODO: replace me by the new common task
	WAIT(30);
	e->move.retention = 0.9;
	WAIT(30);
	e->move.retention = 1;

	int circles = difficulty_value(1,2,3,4);
	int projs_per_circle = 16;
	
	for(int i = 0; i < projs_per_circle; i++) {
		for(int j = 0; j < circles; j++) {
			play_sfx("shot2");

			real angle_offset = -0.01 * i * sign(creal(ARGS.move.velocity));
			cmplx dir = cdir((i - 4) * M_PI / 16 + angle_offset) * cnormalize(ARGS.move.velocity);

			PROJECTILE(
				.proto = pp_wave,
				.pos = e->pos + 30 * dir,
				.color = RGB(1 - 0.2 * j, 0.5, 0.7),
				.move = move_asymptotic_simple(2 * dir, 2 + 2 * j),
			);
		}
		YIELD;
	}

	WAIT(90);
	e->move = move_accelerated(0, 0.05 * I);
	WAIT(40);
	e->move.acceleration = 0;
}

TASK(cardbuster_fairy_move, { Enemy *e; int stops; cmplx *poss; int *move_times; int *pause_times; }) {
	for(int i = 0; i < ARGS.stops-1; i++) {
		WAIT(ARGS.pause_times[i]);
		ARGS.e->move = move_linear((ARGS.poss[i+1] - ARGS.e->pos) / ARGS.move_times[i]);
		WAIT(ARGS.move_times[i]);
		ARGS.e->move = move_stop(0.8);
	}
}
	

TASK(cardbuster_fairy_second_attack, { Enemy *e; }) {
	int count = difficulty_value(40, 60, 80, 100);
	for(int i = 0; i < count; i++) {
		play_sfx_loop("shot1_loop");
		cmplx dir = cnormalize(global.plr.pos - ARGS.e->pos) * cdir(2 * M_TAU / (count + 1) * i);
		real speed = difficulty_value(1.0, 1.2, 1.4, 1.6);
		PROJECTILE(
			.proto = pp_card,
			.pos = ARGS.e->pos + 30 * dir,
			.color = RGB(0, 0.7, 0.5),
			.move = move_asymptotic_simple(speed * dir, 0.4*I),
		);
		YIELD;
	}
}
	

TASK(cardbuster_fairy, { cmplx poss[4]; }) {
	Enemy *e = TASK_BIND(espawn_big_fairy(ARGS.poss[0], ITEMS(.points = 3, .power = 1)));

	int move_times[ARRAY_SIZE(ARGS.poss)-1] = {
		120, 100, 200
	};
	int pause_times[ARRAY_SIZE(ARGS.poss)-1] = {
		0, 80, 100
	};
	int second_attack_delay = 240;

	INVOKE_SUBTASK(cardbuster_fairy_move, e, ARRAY_SIZE(ARGS.poss), ARGS.poss, move_times, pause_times);
	INVOKE_SUBTASK_DELAYED(second_attack_delay, cardbuster_fairy_second_attack, e);

	
	int count = difficulty_value(40, 80, 120, 160);

	WAIT(60);
	for(int i = 0; i < count; i++) {
		play_sfx_loop("shot1_loop");
		cmplx dir = cnormalize(global.plr.pos - e->pos) * cdir(2 * M_TAU / (count + 1) * i);

		for(int j = 0; j < 3; j++) {
			real speed = difficulty_value(1.0, 1.2, 1.4, 1.6) * (1.0 + 0.1 * j);
			real speedup = 0.01*(1.0 + 0.01 * i);
			PROJECTILE(
				.proto = pp_card,
				.pos = e->pos + 30 * dir,
				.color = RGB(0, 0.2, 0.4 + 0.2 * j),
				.move = move_asymptotic_simple(speed * dir, speedup * dir),
			);
		}

		YIELD;
	}

	STALL;
}

TASK(backfire_swirl_move, { BoxedEnemy enemy; }) {
	Enemy *e = TASK_BIND(ARGS.enemy);

	e->move.acceleration = -0.05 * I;
	WAIT(20);
	e->move.acceleration = 0;
	WAIT(40);
	e->move.acceleration = 0.05 * I;
	WAIT(40);
	e->move.acceleration = -0.02 * I;
}
	

TASK(backfire_swirl, { cmplx pos; MoveParams move; }) {
	Enemy *e = TASK_BIND(espawn_swirl(ARGS.pos, ITEMS(.points = 5)));
	e->move = ARGS.move;

	INVOKE_SUBTASK(backfire_swirl_move, ENT_BOX(e));

	WAIT(20);
	int count = difficulty_value(90, 100, 110, 120);
	for(int i = 0; i < count; i++) {
		play_sfx("shot2");
		cmplx dir = cdir(M_PI * rng_real() - M_PI / 2.0 * sign(creal(ARGS.move.velocity)));
		for(int j = 0; j < global.diff; j++) {
			PROJECTILE(
				.proto = pp_wave,
				.pos = e->pos,
				.color = RGB(0.2, 0.2, 1 - 0.2 * j),
				.move = move_asymptotic_simple(2 * dir, 2 + 2 * j),
			);
		}
		WAIT(2);
	}

	STALL;
}

TASK(bigcircle_fairy_move, { BoxedEnemy enemy; cmplx vel; }) {
	Enemy *e = TASK_BIND(ARGS.enemy);

	e->move = move_linear(ARGS.vel);
	WAIT(70);
	e->move = move_stop(0.9);
	WAIT(130);
	e->move = move_linear(-ARGS.vel);
	WAIT(100);
	e->move = move_stop(0.9);
}

TASK(bigcircle_fairy, { cmplx pos; cmplx vel; }) {
	Enemy *e = TASK_BIND(espawn_big_fairy(ARGS.pos, ITEMS(.points = 3, .power = 1)));
	INVOKE_TASK(bigcircle_fairy_move, ENT_BOX(e), ARGS.vel);

	WAIT(80);
	int shots = difficulty_value(3, 4, 6, 7);
	for(int i = 0; i < shots; i++) {

		play_sfx("shot_special1");
		int count = difficulty_value(13, 16, 19, 22);
		real ball_speed = difficulty_value(1.7, 1.9, 2.1, 2.3);

		for(int j = 0; j < count; j++) {
			PROJECTILE(
				.proto = pp_bigball,
				.pos = e->pos,
				.color = RGBA(0, 0.8 - 0.4 * i, 0, 0),
				.move = move_asymptotic_simple(2*cdir(M_TAU / count * j + 3 * i), 3 * sin(6 * M_PI / count * j))
			);

			if(global.diff > D_Easy) {
				PROJECTILE(
					.proto = pp_ball,
					.pos = e->pos,
					.color = RGBA(0, 0.3 * i, 0.4, 0),
					.move = move_asymptotic_simple(ball_speed*cdir(3*(j+rng_real())),
						I * 5 * sin(6 * M_PI / count * j))
				);
			}
		}

		WAIT(20);
	}
}


TASK(explosive_swirl_explosion, { BoxedEnemy enemy; }) {
	Enemy *e = TASK_BIND(ARGS.enemy);

	int count = difficulty_value(10, 20, 30, 40);
	cmplx aim = cnormalize(global.plr.pos - e->pos);

	real speed = 1.5 * difficulty_value(1.4, 1.7, 2.0, 2.3);

	for(int i = 0; i < count; i++) {
		cmplx dir = cdir(M_TAU * i / count) * aim;
		PROJECTILE(
			.proto = pp_ball,
			.pos = e->pos,
			.color = RGB(0.1 + 0.6 * (i&1), 0.2, 1 - 0.6 * (i&1)),
			.move = move_accelerated(speed * dir, 0.001*dir),
		);
	}

	play_sfx("shot1");
	enemy_kill(e);
}

TASK(explosive_swirl, { cmplx pos; MoveParams move; }) {
	Enemy *e = TASK_BIND(espawn_swirl(ARGS.pos, ITEMS(.power = 1)));
	e->move = ARGS.move;

	INVOKE_TASK_WHEN(&e->events.killed, explosive_swirl_explosion, ENT_BOX(e));
	if(global.diff >= D_Normal) {
		INVOKE_TASK_DELAYED(100, explosive_swirl_explosion, ENT_BOX(e));
	}
}

TASK(supercard_proj, { cmplx pos; Color color; MoveParams move_start; MoveParams move_split; int split_time; }) {
	Projectile *p = TASK_BIND(PROJECTILE(
		.proto = pp_card,
		.pos = ARGS.pos,
		.color = &ARGS.color,
	));
	p->move = ARGS.move_start;
	
	WAIT(ARGS.split_time);
	p->color = *RGB(p->color.b, 0.2, p->color.g);
	play_sfx_ex("redirect", 10, false);
	spawn_projectile_highlight_effect(p);
	p->move = ARGS.move_split;
}

TASK(supercard_fairy_move, { BoxedEnemy enemy; }) {
	Enemy *e = TASK_BIND(ARGS.enemy);

	WAIT(50);
	e->move.retention = 0.7;
	WAIT(20);
	e->move.retention = 1;
	WAIT(380);
	e->move = move_linear(-I);
}

TASK(supercard_fairy, { cmplx pos; MoveParams move; }) {
	Enemy *e = TASK_BIND(espawn_big_fairy(ARGS.pos, ITEMS(.points = 5)));
	e->move = ARGS.move;
	INVOKE_TASK(supercard_fairy_move, ENT_BOX(e));

	for(int repeat = 0;; repeat++) {
		WAIT(70);
		int count = difficulty_value(20, 40, 40, 50);
		int fan = difficulty_value(1, 1, 2, 2);

		for(int i = 0; i < count; i++) { 
			play_sfx_ex("shot1",5,false);

			cmplx dir = cdir(M_TAU / count * i + repeat * M_PI/4);
			
			for(int j = -fan; j <= fan; j++) {
				MoveParams move_split = move_accelerated(dir, 0.01 * (1 + j * I) * dir);
				move_split.retention = 0.99;

				INVOKE_TASK(supercard_proj,
					.pos = e->pos + 30 * dir,
					.color = *RGB(0, 0.4, 1 - 0.5 * psin(i / 20.0)),
					.move_start = move_linear(0.01 * dir),
					.move_split = move_split, 
					.split_time = 100 - i
				);
			}

			YIELD;
		}
	}
}



TASK(scythe_post_mid, { cmplx pos; int fleetime; }) {
	Enemy *e = TASK_BIND(espawn_big_fairy(ARGS.pos, NULL));
	e->flags |= EFLAG_NO_HIT | EFLAG_INVULNERABLE | EFLAG_NO_AUTOKILL;

	cmplx anchor = VIEWPORT_W / 2.0 + I * VIEWPORT_H / 3.0;

	e->move = move_towards(anchor, 0.003 + 0.02 * I); 

	real speed = difficulty_value(1.1, 1.0, 0.9, 0.8);

	WAIT(90);

	for(int i = 0; i < ARGS.fleetime - 210; i++, YIELD) {
		cmplx shotorg = e->pos + 80 * cdir(-0.1 * i);
		cmplx shotdir = cdir(-0.3563 * i);

		struct projentry { ProjPrototype *proj; char *snd; } projs[] = {
			{ pp_ball,       "shot1"},
			{ pp_bigball,    "shot1"},
			{ pp_soul,       "shot_special1"},
			{ pp_bigball,    "shot1"},
		};

		struct projentry *pe = &projs[i % (sizeof(projs)/sizeof(struct projentry))];

		double ca = creal(e->args[1]) + i/60.0;
		Color *c = RGB(cos(ca), sin(ca), cos(ca+2.1));

		play_sfx_ex(pe->snd, 3, true);

		PROJECTILE(
			.proto = pe->proj,
			.pos = shotorg,
			.color = c,
			.move = move_asymptotic_simple(speed * shotdir, 5 * sin(i / 150.0))
		);
	}

	for(int i = 0; i < 120; i++, YIELD) {
		stage_clear_hazards(CLEAR_HAZARDS_ALL);
	}

	if(ARGS.fleetime >= 300) {
		spawn_items(e->pos, ITEM_LIFE, 1);
	}

	enemy_kill(e);
}


TASK(ensure_scythe_spawn, { int duration; }) {
	for(int i = 0; i < ARGS.duration; i++) {
		if(!global.boss) {
			INVOKE_TASK(scythe_post_mid, .pos = VIEWPORT_W / 2.0, .fleetime = ARGS.duration - i);
			return;
		}
		YIELD;
	}
}

	
DEFINE_EXTERN_TASK(stage4_timeline) {
	INVOKE_TASK_DELAYED(70, splasher_fairy, VIEWPORT_H / 4.0 * 3 * I, 1);
	INVOKE_TASK_DELAYED(70, splasher_fairy, VIEWPORT_W + VIEWPORT_H / 4.0 * 3 * I, -1);

	for(int i = 0; i < 15; i++) {
		real phase = 2 * i / M_PI * (1 - 2 * (i&1));
		cmplx pos = VIEWPORT_W * 0.5 * (1 + 0.5 * sin(phase)) - 32 * I;
		INVOKE_TASK_DELAYED(300 + 10 * i, fodder_fairy, .pos = pos, .move = move_linear(2 * I * cdir(0.1 * phase)));
	}

	for(int i = 0; i < 5; i++) {
		cmplx pos = VIEWPORT_W * (i & 1);
		cmplx vel = 2 * cdir(M_PI / 2 * (0.2 + 0.6 * rng_real() + (i & 1)));

		INVOKE_TASK_DELAYED(500 + 10 * i, partcircle_fairy, .pos = pos, .move = move_linear(vel));
	}
				     

	for(int i = 0; i < 8; i++) {
		cmplx pos0 = VIEWPORT_W * (1.0 + 2.0 * (i & 1)) / 4.0;
		cmplx pos1 = VIEWPORT_W * (1.0 + 4.0 * (i & 1)) / 6.0 + 100 * I;
		cmplx pos2 = VIEWPORT_W * (1.0 + 2.0 * ((i+1) & 1)) / 4.0 + 300 * I;
		cmplx pos3 = VIEWPORT_W * 0.5 + (VIEWPORT_H + 200) * I;
		
		INVOKE_TASK_DELAYED(600 + 100 * i, cardbuster_fairy, .poss = { 
			pos0, pos1, pos2, pos3
		});
	}
			

	INVOKE_TASK_DELAYED(1800, backfire_swirl, .pos = VIEWPORT_H / 2.0 * I, .move = move_linear(0.3));
	INVOKE_TASK_DELAYED(1800, backfire_swirl, .pos = VIEWPORT_W + VIEWPORT_H / 2.0 * I, .move = move_linear(-0.3));

	for(int i = 0; i < 36; i++) {
		real phase = 2*i/M_PI * (1 - 2*(i&1));
		cmplx pos = -24 + I * psin(phase) * 300;
		INVOKE_TASK_DELAYED(2060 + 15 * i, fodder_fairy, .pos = pos, .move = move_linear(2 * cdir(0.005 * phase)));
	}

	for(int i = 0; i < 2; i++) {
		cmplx pos = VIEWPORT_W / 2.0 + 200 * rng_sreal();
		INVOKE_TASK_DELAYED(2000 + 200 * i, bigcircle_fairy, .pos = pos, .vel = 2.0 * I);
	}

	for(int i = 0; i < 40; i++) {
		cmplx pos = VIEWPORT_W + I * (20 + VIEWPORT_H / 3.0 * rng_real());
		INVOKE_TASK_DELAYED(2600 + 10 * i, explosive_swirl, .pos = pos, .move = move_linear(-3));
	}

/* 	AT(3200) */
/* 		global.boss = create_kurumi_mid(); */

 	int midboss_time = STAGE4_MIDBOSS_MUSIC_TIME;

/* 	AT(3201) { */
/* 		if(global.boss) { */
/* 			global.timer += fmin(midboss_time, global.frames - global.boss->birthtime) - 1; */
/* 		} */
/* 	} */

	INVOKE_TASK_DELAYED(3201, ensure_scythe_spawn, .duration = midboss_time -1);
/* 	FROM_TO(3201, 3201 + midboss_time - 1, 1) { */
/* 		if(!global.enemies.first) { */
/* 			create_enemy4c(VIEWPORT_W/2+160.0*I, ENEMY_IMMUNE, scythe_draw, scythe_post_mid, 0, 1+0.2*I, 0+1*I, 3201 + midboss_time - global.timer); */
/* 		} */
/* 	} */
			     
	for(int i = 0; i < 20; i++) {
		real phase = 2 * i/M_PI;
		cmplx pos = VIEWPORT_W * (i&1) + I * 120 * psin(phase);
		INVOKE_TASK_DELAYED(2060 + midboss_time + 15 * i, fodder_fairy, .pos = pos, .move = move_linear(2 - 4 * (i & 1) + I));
	}
			     
	for(int i = 0; i < 11; i++) {
		cmplx pos = VIEWPORT_W * (i & 1);
		cmplx vel = 2 * cdir(M_PI / 2 * (0.2 + 0.6 * rng_real() + (i & 1)));

		INVOKE_TASK_DELAYED(3350 + midboss_time + 60 * i, partcircle_fairy, .pos = pos, .move = move_linear(vel));
	}

	for(int i = 0; i < 4; i++) {
		cmplx pos0 = VIEWPORT_W * (1.0 + 2.0 * (i & 1)) / 4.0;
		cmplx pos1 = VIEWPORT_W * (0.5 + 0.4 * (1 - 2 * (i & 1))) + 100.0 * I;
		cmplx pos2 = VIEWPORT_W * (1.0 + 2.0 * ((i + 1) & 1)) / 4.0 + 300.0*I;
		cmplx pos3 = VIEWPORT_W * 0.5 - 200 * I;
		
		INVOKE_TASK_DELAYED(3550 + midboss_time + 200 * i, cardbuster_fairy, .poss = { 
			pos0, pos1, pos2, pos3
		});
	}
		
	INVOKE_TASK_DELAYED(3800 + midboss_time, supercard_fairy,
			    .pos = VIEWPORT_W / 2.0,
			    .move = move_linear(4.0 * I)
	);

	int swirl_delay = difficulty_value(85, 75, 65, 55);
	int swirl_count = 300 / swirl_delay;
	for(int i = 0; i < swirl_count; i++) {
		cmplx pos = VIEWPORT_W * (i & 1) + 100 * I;
		INVOKE_TASK_DELAYED(4300 + midboss_time + swirl_delay * i, backfire_swirl, .pos = pos, .move = move_linear(rng_real() * (1 - 2 * (i & 1))));
	}

	for(int i = 0; i < 40; i++) {
		cmplx pos = VIEWPORT_W * (i & 1) + I * (20.0 + VIEWPORT_H/3.0 * rng_real());
		cmplx vel = 3 * (1 - 2 * (i & 1)) + I;
		INVOKE_TASK_DELAYED(4800 + midboss_time + 10 * i, explosive_swirl, .pos = pos, .move = move_linear(vel));
	}
/* 	AT(5300 + midboss_time) { */
/* 		stage_unlock_bgm("stage4"); */
/* 		stage4_bg_redden_corridor(); */
/* 		global.boss = create_kurumi(); */
/* 	} */

/* 	AT(5400 + midboss_time) { */
/* 		stage_unlock_bgm("stage4boss"); */
/* 		stage4_dialog_post_boss(); */
/* 	} */

/* 	AT(5405 + midboss_time) { */
/* 		stage_finish(GAMEOVER_SCORESCREEN); */
/* 	} */
/* } */
	WAIT(5400 + midboss_time);
	stage_finish(GAMEOVER_SCORESCREEN);
}

