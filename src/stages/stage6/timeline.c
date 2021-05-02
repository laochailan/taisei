/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "timeline.h"
#include "stage6.h"
#include "draw.h"
#include "background_anim.h"
#include "enemy_classes.h""

#include "elly.h"
#include "nonspells/nonspells.h"

#include "common_tasks.h"

MODERNIZE_THIS_FILE_AND_REMOVE_ME

TASK(boss_appear_stub, NO_ARGS) {
	log_warn("FIXME");
}

static void stage6_dialog_pre_boss(void) {
	PlayerMode *pm = global.plr.mode;
	Stage6PreBossDialogEvents *e;
	INVOKE_TASK_INDIRECT(Stage6PreBossDialog, pm->dialog->Stage6PreBoss, &e);
	INVOKE_TASK_WHEN(&e->boss_appears, boss_appear_stub);
	INVOKE_TASK_WHEN(&e->music_changes, common_start_bgm, "stage6boss_phase1");
}

static void stage6_dialog_pre_final(void) {
	PlayerMode *pm = global.plr.mode;
	INVOKE_TASK_INDIRECT(Stage6PreFinalDialog, pm->dialog->Stage6PreFinal);
}

TASK(hacker_fairy, { cmplx pos; MoveParams move; }) {
	Enemy *e = TASK_BIND(espawn_huge_fairy(ARGS.pos, ITEMS(.points = 4, .power = 3)));
	e->move = ARGS.move;

	WAIT(70);
	e->move = move_stop(0.5);

	WAIT(30);
	int duration = difficulty_value(220, 260, 300, 340);
	int step = 3;
	
	for(int i = 0; i < duration/step; i++, WAIT(step)) {
		play_sfx_loop("shot1_loop");
		for(int j = 0; j < 6; j++) {
			cmplx dir = sin(i * 0.2) * cdir(0.3 * (j / 2 - 1)) * (1 - 2 * (i&1));

			cmplx vel = (-0.5 * sin(global.frames + i * 46752 + 16463 * j + 467 * sin(global.frames*i*j))) * global.diff + creal(dir) + 2.0 * I;

			PROJECTILE(
				.proto = pp_wave,
				.pos = e->pos + 120 * dir,
				.color = RGB(1.0, 0.2 - 0.01 * i, 0.0),
				.move = move_linear(vel)
			);
		}
	}

	WAIT(60);
	e->move = move_linear(-ARGS.move.velocity);
}

TASK(side_fairy, { cmplx pos; MoveParams move; cmplx direction; real index; }) {
	Enemy *e = TASK_BIND(espawn_fairy_blue(ARGS.pos, ITEMS(.points = 4, .power = 3)));
	e->move = ARGS.move;

	WAIT(60);
	e->move = move_stop(0.5);

	WAIT(10);

	int count = difficulty_value(25, 35, 45, 55); 
	real speed = difficulty_value(0.9, 1.1, 1.3, 1.5);
	play_sfx_ex("shot1", 4, true);
	for(int i = 0; i < count; i++) {
		PROJECTILE(
			.proto = (i % 2 == 0) ? pp_rice : pp_flea,
			.pos = e->pos + 5 * (i/2) * ARGS.direction,
			.color = RGB(0.1 * ARGS.index, 0.5, 1),
			.move = move_accelerated(I * (1.0 - 2.0 * (i & 1)) * speed,
				0.001 * (i/2) * ARGS.direction)
		);
	}

	WAIT(50);
	e->move = ARGS.move;
}

static int stage6_flowermine(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 4, ITEM_POWER, 3);
		return 1;
	}

	e->pos += e->args[0];

	FROM_TO(70, 200, 1)
		e->args[0] += 0.07*cexp(I*carg(e->args[1]-e->pos));

	FROM_TO(0, 1000, 7-global.diff) {
		PROJECTILE(
			.proto = pp_rice,
			.pos = e->pos + 40*cexp(I*0.6*_i+I*carg(e->args[0])),
			.color = RGB(1-psin(_i), 0.3, psin(_i)),
			.rule = wait_proj,
			.args = {
				I*cexp(I*0.6*_i)*(0.7+0.3*global.diff),
				200
			},
			.angle = 0.6*_i,
		);
		play_sfx("shot1");
	}

	return 1;
}

static int scythe_intro(Enemy *e, int t) {
	if(t < 0) {
		scythe_common(e, t);
		return 0;
	}

	TIMER(&t);

	GO_TO(e,VIEWPORT_W/2+200.0*I, 0.05);


	FROM_TO(60, 119, 1) {
		e->args[1] -= 0.00333333*I;
		e->args[2] -= 0.007;
	}

	scythe_common(e, t);
	return 1;
}

static void elly_intro(Boss *b, int t) {
	TIMER(&t);

	if(global.stage->type == STAGE_SPELL) {
		GO_TO(b, BOSS_DEFAULT_GO_POS, 0.1);

		AT(0) {
			create_enemy3c(VIEWPORT_W+200.0*I, ENEMY_IMMUNE, scythe_draw, scythe_intro, 0, 1+0.2*I, 1);
		}

		return;
	}

	GO_TO(b, BOSS_DEFAULT_GO_POS, 0.01);

	AT(200) {
		create_enemy3c(VIEWPORT_W+200+200.0*I, ENEMY_IMMUNE, scythe_draw, scythe_intro, 0, 1+0.2*I, 1);
	}

	AT(300)
		stage6_dialog_pre_boss();
}

Boss* stage6_spawn_elly(cmplx pos) {
	Boss *b = create_boss("Elly", "elly", pos);
	boss_set_portrait(b, "elly", NULL, "normal");
	b->global_rule = elly_global_rule;
	return b;
}

static void elly_insert_interboss_dialog(Boss *b, int t) {
	stage6_dialog_pre_final();
}

static void elly_begin_toe(Boss *b, int t) {
	TIMER(&t);

	AT(1) {
		start_fall_over();
		stage_unlock_bgm("stage6boss_phase2");
		stage_start_bgm("stage6boss_phase3");
	}
}

static void elly_goto_center(Boss *b, int t) {
	GO_TO(b, BOSS_DEFAULT_GO_POS, 0.05);
}

static Boss* create_elly(void) {
	Boss *b = stage6_spawn_elly(-200.0*I);

	boss_add_attack(b, AT_Move, "Catch the Scythe", 6, 0, elly_intro, NULL);
	boss_add_attack(b, AT_Normal, "Frequency", 40, 50000, elly_frequency, NULL);
	boss_add_attack_from_info(b, &stage6_spells.scythe.occams_razor, false);
	boss_add_attack(b, AT_Normal, "Frequency2", 40, 50000, elly_frequency2, NULL);
	boss_add_attack_from_info(b, &stage6_spells.scythe.orbital_clockwork, false);
	boss_add_attack_from_info(b, &stage6_spells.scythe.wave_theory, false);
	boss_add_attack(b, AT_Move, "Paradigm Shift", 3, 0, elly_paradigm_shift, NULL);
	boss_add_attack_from_info(b, &stage6_spells.baryon.many_world_interpretation, false);
	boss_add_attack(b, AT_Normal, "Baryon", 50, 55000, elly_baryonattack, NULL);
	boss_add_attack_from_info(b, &stage6_spells.baryon.wave_particle_duality, false);
	boss_add_attack_from_info(b, &stage6_spells.baryon.spacetime_curvature, false);
	boss_add_attack(b, AT_Normal, "Baryon2", 50, 55000, elly_baryonattack2, NULL);
	boss_add_attack_from_info(b, &stage6_spells.baryon.higgs_boson_uncovered, false);
	boss_add_attack_from_info(b, &stage6_spells.extra.curvature_domination, false);
	boss_add_attack(b, AT_Move, "Explode", 4, 0, elly_baryon_explode, NULL);
	boss_add_attack(b, AT_Move, "Move to center", 4, 0, elly_goto_center, NULL);
	boss_add_attack(b, AT_Immediate, "Final dialog", 0, 0, elly_insert_interboss_dialog, NULL);
	boss_add_attack(b, AT_Move, "ToE transition", 7, 0, elly_begin_toe, NULL);
	boss_add_attack_from_info(b, &stage6_spells.final.theory_of_everything, false);
	boss_start_attack(b, b->attacks);

	return b;
}

DEFINE_EXTERN_TASK(stage6_timeline) {

	INVOKE_TASK_DELAYED(100, hacker_fairy, .pos = VIEWPORT_W / 2.0, .move = move_linear(2.0 * I));

	for(int i = 0; i < 20; i++) {
		INVOKE_TASK_DELAYED(500 + 10 * i, side_fairy,
			.pos = VIEWPORT_W * (i & 1),
			.move = move_linear(2.0 * I + 0.1 * (1 - 2 * (i & 1))),
			.direction = 1 - 2 * (i & 1),
			.index = i
		);
	}

	for(int i = 0; i < 22; i++) {
		cmplx pos = VIEWPORT_W / 2.0 + (1 - 2*(i&1)) * 20 * (i % 10);
		INVOKE_TASK_DELAYED(720 + 10 * i, side_fairy,
			.pos = pos,
			.move = move_linear(2.0 * I + (1 - 2 * (i & 1))),
			.direction = I * cnormalize(global.plr.pos - pos) * (1 - 2 * (i & 1)),
			.index = i * psin(i)
		);
	}

	/*FROM_TO(1380, 1660, 20)
		create_enemy2c(200.0*I, 600, Fairy, stage6_flowermine, 2*cexp(0.5*I*M_PI/9*_i)+1, 0);

	FROM_TO(1600, 2000, 20)
		create_enemy3c(VIEWPORT_W/2, 600, Fairy, stage6_flowermine, 2*cexp(0.5*I*M_PI/9*_i+I*M_PI/2)+1.0*I, VIEWPORT_H/2*I+VIEWPORT_W, 1);

	AT(2300)
		create_enemy3c(200.0*I-200, ENEMY_IMMUNE, scythe_draw, scythe_mid, 1, 0.2*I, 1);

	AT(3800) {
		stage_unlock_bgm("stage6");
		global.boss = create_elly();
	}

	AT(3805) {
		stage_unlock_bgm("stage6boss_phase3");
		stage_finish(GAMEOVER_SCORESCREEN);
	}*/
}
