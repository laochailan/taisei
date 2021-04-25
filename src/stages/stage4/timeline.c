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





static int stage4_bigcircle(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 3, ITEM_POWER, 1);

		return 1;
	}

	FROM_TO(0, 70, 1)
		e->pos += e->args[0];

	FROM_TO(200, 300, 1)
		e->pos -= e->args[0];


	FROM_TO(80,100+30*global.diff,20) {
		play_sound("shot_special1");
		int i;
		int n = 10+3*global.diff;
		for(i = 0; i < n; i++) {
			PROJECTILE(
				.proto = pp_bigball,
				.pos = e->pos,
				.color = RGBA(0, 0.8 - 0.4 * _i, 0, 0),
				.rule = asymptotic,
				.args = {
					2*cexp(2.0*I*M_PI/n*i+I*3*_i),
					3*sin(6*M_PI/n*i)
				},
			);

			if(global.diff > D_Easy) {
				PROJECTILE(
					.proto = pp_ball,
					.pos = e->pos,
					.color = RGBA(0, 0.3 * _i, 0.4, 0),
					.rule = asymptotic,
					.args = {
						(1.5+global.diff*0.2)*cexp(I*3*(i+frand())),
						I*5*sin(6*M_PI/n*i)
					},
				);
			}
		}
	}
	return 1;
}

static int stage4_explosive(Enemy *e, int t) {
	if(t == EVENT_KILLED || (t >= 100 && global.diff >= D_Normal)) {
		int i;

		if(t == EVENT_KILLED)
			spawn_items(e->pos, ITEM_POWER, 1);

		int n = 10*global.diff;
		cmplx phase = global.plr.pos-e->pos;
		phase /= cabs(phase);

		for(i = 0; i < n; i++) {
			double angle = 2*M_PI*i/n+carg(phase);
			PROJECTILE(
				.proto = pp_ball,
				.pos = e->pos,
				.color = RGB(0.1+0.6*(i&1), 0.2, 1-0.6*(i&1)),
				.rule = accelerated,
				.args = {
					1.5*(1.1+0.3*global.diff)*cexp(I*angle),
					0.001*cexp(I*angle)
				}
			);
		}

		play_sound("shot1");
		return ACTION_DESTROY;
	}

	e->pos += e->args[0];

	return 1;
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

static int splitcard(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	if(t == creal(p->args[2])) {
		// projectile_set_prototype(p, pp_bigball);
		p->color = *RGB(p->color.b, 0.2, p->color.g);
		play_sound_ex("redirect", 10, false);
		spawn_projectile_highlight_effect(p);
	}

	if(t > creal(p->args[2])) {
		p->args[0] += 0.01*p->args[3];
		asymptotic(p, t);
	} else {
		p->angle = carg(p->args[0]);
	}

	return ACTION_NONE; // asymptotic(p, t);
}

static int stage4_supercard(Enemy *e, int t) {
	int time = t % 150;

	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 5);
		return 1;
	}

	e->pos += e->args[0];

	FROM_TO(50, 70, 1) {
		e->args[0] *= 0.7;
	}

	if(t > 450) {
		e->pos -= I;
		return 1;
	}

	__timep = &time;

	FROM_TO(70, 70+20*global.diff, 1) {
		play_sound_ex("shot1",5,false);

		cmplx n = cexp(I*(2*M_PI/20.0*_i + (t / 150) * M_PI/4));
		for(int i = -1; i <= 1 && t; i++) {
			PROJECTILE(
				.proto = pp_card,
				.pos = e->pos + 30*n,
				.color = RGB(0,0.4,1-_i/40.0),
				.rule = splitcard,
				.args = {1*n, 0.1*_i, 100-time+70, 2*I*i*n}
			);
		}
	}

	return 1;
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

static int scythe_post_mid(Enemy *e, int t) {
	TIMER(&t);

	int fleetime = creal(e->args[3]);

	if(t == EVENT_DEATH) {
		if(fleetime >= 300) {
			spawn_items(e->pos, ITEM_LIFE, 1);
		}

		return 1;
	}

	if(t < 0) {
		return 1;
	}

	AT(fleetime) {
		return ACTION_DESTROY;
	}

	double scale = fmin(1.0, t / 60.0) * (1.0 - clamp((t - (fleetime - 60)) / 60.0, 0.0, 1.0));
	double alpha = scale * scale;
	double spin = (0.2 + 0.2 * (1.0 - alpha)) * 1.5;

	cmplx opos = VIEWPORT_W/2+160*I;
	double targ = (t-300) * (0.5 + psin(t/300.0));
	double w = fmin(0.15, 0.0001*targ);

	cmplx pofs = 150*cos(w*targ+M_PI/2.0) + I*80*sin(2*w*targ);
	pofs += ((VIEWPORT_W/2+VIEWPORT_H/2*I - opos) * (global.diff - D_Easy)) / (D_Lunatic - D_Easy);

	e->pos = opos + pofs * (1.0 - clamp((t - (fleetime - 120)) / 60.0, 0.0, 1.0)) * smooth(smooth(scale));
	e->args[2] = 0.5 * scale + (1.0 - alpha) * I;
	e->args[1] = creal(e->args[1]) + spin * I;

	FROM_TO(90, fleetime - 120, 1) {
		cmplx shotorg = e->pos+80*cexp(I*creal(e->args[1]));
		cmplx shotdir = cexp(I*creal(e->args[1]));

		struct projentry { ProjPrototype *proj; char *snd; } projs[] = {
			{ pp_ball,       "shot1"},
			{ pp_bigball,    "shot1"},
			{ pp_soul,       "shot_special1"},
			{ pp_bigball,    "shot1"},
		};

		struct projentry *pe = &projs[_i % (sizeof(projs)/sizeof(struct projentry))];

		double ca = creal(e->args[1]) + _i/60.0;
		Color *c = RGB(cos(ca), sin(ca), cos(ca+2.1));

		play_sound_ex(pe->snd, 3, true);

		PROJECTILE(
			.proto = pe->proj,
			.pos = shotorg,
			.color = c,
			.rule = asymptotic,
			.args = {
				(1.2-0.1*global.diff)*shotdir,
				5 * sin(t/150.0)
			},
		);
	}

	FROM_TO(fleetime - 120, fleetime, 1) {
		stage_clear_hazards(CLEAR_HAZARDS_ALL);
	}

	scythe_common(e, t);
	return 1;
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

DEFINE_EXTERN_TASK(stage4_timeline) {
	INVOKE_TASK_DELAYED(70, splasher_fairy, VIEWPORT_H/4 * 3 * I, 1);
	INVOKE_TASK_DELAYED(70, splasher_fairy, VIEWPORT_W + VIEWPORT_H/4 * 3 * I, -1);

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

/* 	FROM_TO(2000, 2400, 200) */

/* 	FROM_TO(2600, 3000, 10) */
/* 		create_enemy1c(20.0*I+VIEWPORT_H/3*I*frand()+VIEWPORT_W, 100, Swirl, stage4_explosive, -3); */

/* 	AT(3200) */
/* 		global.boss = create_kurumi_mid(); */

 	int midboss_time = STAGE4_MIDBOSS_MUSIC_TIME;

/* 	AT(3201) { */
/* 		if(global.boss) { */
/* 			global.timer += fmin(midboss_time, global.frames - global.boss->birthtime) - 1; */
/* 		} */
/* 	} */

/* 	FROM_TO(3201, 3201 + midboss_time - 1, 1) { */
/* 		if(!global.enemies.first) { */
/* 			create_enemy4c(VIEWPORT_W/2+160.0*I, ENEMY_IMMUNE, scythe_draw, scythe_post_mid, 0, 1+0.2*I, 0+1*I, 3201 + midboss_time - global.timer); */
/* 		} */
/* 	} */
			     
	for(int i = 0; i < 20; i++) {
		real phase = 2 * i/M_PI;
		cmplx pos = I* VIEWPORT_W * (i&1) * 120 * psin(phase);
		INVOKE_TASK_DELAYED(2060 + midboss_time + 15 * i, fodder_fairy, .pos = pos, .move = move_linear(2 - 4 * (i & 1) + I));
	}
			     
	for(int i = 0; i < 11; i++) {
		cmplx pos = VIEWPORT_W * (i & 1);
		cmplx vel = 2 * cdir(M_PI / 2 * (0.2 + 0.6 * rng_real() + (i & 1)));

		INVOKE_TASK_DELAYED(3350 + midboss_time + 60 * i, partcircle_fairy, .pos = pos, .move = move_linear(vel));
	}

	for(int i = 0; i < 4; i++) {
		cmplx pos0 = VIEWPORT_W * (1.0 + 2.0 * (i & 1)) / 4.0;
		cmplx pos1 = VIEWPORT_W * (0.5 + 0.4 * (1 - 2 * (i & 1))) + 100.0 * I; // TODO: ???
		cmplx pos2 = VIEWPORT_W * (1.0 + 2.0 * ((i + 1) & 1)) / 4.0 + 300.0*I;
		cmplx pos3 = VIEWPORT_W * 0.5 - 200 * I;
		
		INVOKE_TASK_DELAYED(3550 + midboss_time + 200 * i, cardbuster_fairy, .poss = { 
			pos0, pos1, pos2, pos3
		});
	}
		

/* 	AT(3800 + midboss_time) */
/* 		create_enemy1c(VIEWPORT_W/2, 9000, BigFairy, stage4_supercard, 4.0*I); */

	int swirl_delay = difficulty_value(85, 75, 65, 55);
	int swirl_count = 300 / swirl_delay;
	for(int i = 0; i < swirl_count; i++) {
		cmplx pos = VIEWPORT_W * (i & 1) + 100 * I;
		INVOKE_TASK_DELAYED(4300 + midboss_time + swirl_delay * i, backfire_swirl, .pos = pos, .move = move_linear(rng_real() * (1 - 2 * (i & 1))));
	}

/* 	FROM_TO(4800 + midboss_time, 5200 + midboss_time, 10) */
/* 		create_enemy1c(20.0*I+I*VIEWPORT_H/3*frand()+VIEWPORT_W*(_i&1), 100, Swirl, stage4_explosive, (1-2*(_i&1))*3+I); */

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

