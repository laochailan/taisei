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
#include "background_anim.h"
#include "nonspells/nonspells.h"
#include "spells/spells.h"

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

static int stage5_greeter(Enemy *e, int t) {
	TIMER(&t)
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 2, ITEM_POWER, 2);
		return 1;
	}

	e->moving = true;
	e->dir = creal(e->args[0]) < 0;

	FROM_TO(0, 50, 1) {
		GO_TO(e, e->pos0 + e->args[0] * 50, 0.05);
	}

	if(t > 200)
		e->pos += e->args[0];

	FROM_TO(80, 180, 20) {
		for(int i = -(int)global.diff; i <= (int)global.diff; i++) {
			PROJECTILE(
				.proto = pp_bullet,
				.pos = e->pos,
				.color = RGB(0.0, 0.0, 1.0),
				.rule = asymptotic,
				.args = {
					(3.5+(global.diff == D_Lunatic))*cexp(I*carg(global.plr.pos-e->pos) + 0.06*I*i),
					5
				}
			);
		}

		play_sound("shot1");
	}

	return 1;
}

static int stage5_lightburst(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 4, ITEM_POWER, 2);
		return 1;
	}

	FROM_TO(0, 70, 1) {
		GO_TO(e, e->pos0 + e->args[0] * 70, 0.05);
	}

	if(t > 200)
		e->pos += e->args[0];

	FROM_TO_SND("shot1_loop", 20, 300, 5) {
		int c = 5+global.diff;
		for(int i = 0; i < c; i++) {
			cmplx n = cexp(I*carg(global.plr.pos) + 2.0*I*M_PI/c*i);
			PROJECTILE(
				.proto = pp_ball,
				.pos = e->pos + 50*n*cexp(-0.4*I*_i*global.diff),
				.color = RGB(0.3, 0, 0.7),
				.rule = asymptotic,
				.args = { 3*n, 3 }
			);
		}

		play_sound("shot2");
	}

	return 1;
}

static int stage5_swirl(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 1);
		return 1;
	}

	if(t > creal(e->args[1]) && t < cimag(e->args[1]))
		e->args[0] *= e->args[2];

	e->pos += e->args[0];

	FROM_TO(0, 400, 26-global.diff*4) {
		for(int i = 1; i >= -1; i -= 2) {
			PROJECTILE(
				.proto = pp_bullet,
				.pos = e->pos,
				.color = RGB(0.3, 0.4, 0.5),
				.rule = asymptotic,
				.args = { i*2*e->args[0]*I/cabs(e->args[0]), 3 }
			);
		}

		play_sound("shot1");
	}

	return 1;
}

static int stage5_limiter(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 4, ITEM_POWER, 4);
		return 1;
	}

	e->pos += e->args[0];

	FROM_TO_SND("shot1_loop", 0, 1200, 3) {
		uint f = PFLAG_NOSPAWNFADE;
		double base_angle = carg(global.plr.pos - e->pos);

		for(int i = 1; i >= -1; i -= 2) {
			double a = i * 0.2 - 0.1 * (global.diff / 4) + i * 3.0 / (_i + 1);
			cmplx aim = cexp(I * (base_angle + a));

			PROJECTILE(
				.proto = pp_rice,
				.pos = e->pos,
				.color = RGB(0.5,0.1,0.2),
				.rule = asymptotic,
				.args = { 10*aim, 2 },
				.flags = f,
			);
		}
	}

	return 1;
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

static int stage5_miner(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 2);
		return 1;
	}

	e->pos += e->args[0];

	FROM_TO(0, 600, 5-global.diff/2) {
		tsrand_fill(2);
		PROJECTILE(
			.proto = pp_rice,
			.pos = e->pos + 20*cexp(2.0*I*M_PI*afrand(0)),
			.color = RGB(0,0,cabs(e->args[0])),
			.rule = linear,
			.args = { cexp(2.0*I*M_PI*afrand(1)) }
		);
		play_sfx_ex("shot3", 0, false);
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
	
	stage5_bg_raise_lightning();

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

static int stage5_lightburst2(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 4, ITEM_POWER, 4);
		return 1;
	}

	FROM_TO(0, 70, 1) {
		GO_TO(e, e->pos0 + e->args[0] * 70, 0.05);
	}

	if(t > 200)
		e->pos += e->args[0];

	FROM_TO_SND("shot1_loop", 20, 170, 5) {
		int i;
		int c = 4+global.diff-(global.diff==D_Easy);
		for(i = 0; i < c; i++) {
			tsrand_fill(2);
			cmplx n = cexp(I*carg(global.plr.pos-e->pos) + 2.0*I*M_PI/c*i);
			PROJECTILE(
				.proto = pp_bigball,
				.pos = e->pos + 50*n*cexp(-1.0*I*_i*global.diff),
				.color = RGB(0.3, 0, 0.7+0.3*(_i&1)),
				.rule = asymptotic,
				.args = {
					2.5*n+0.25*global.diff*afrand(0)*cexp(2.0*I*M_PI*afrand(1)),
					3
				}
			);
		}

		play_sound("shot2");
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

void stage5_events(void) {
	TIMER(&global.timer);

	AT(0) {
		stage_start_bgm("stage5");
		stage_set_voltage_thresholds(255, 480, 860, 1250);
	}

	FROM_TO(60, 150, 15) {
		create_enemy1c(VIEWPORT_W*(_i&1)+70.0*I+50*_i*I, 400, Fairy, stage5_greeter, 3-6*(_i&1));
	}

	FROM_TO(270, 320, 40)
		create_enemy1c(VIEWPORT_W/4+VIEWPORT_W/2*_i, 2000, BigFairy, stage5_lightburst, 2.0*I);

	FROM_TO(400, 600, 10) {
		tsrand_fill(2);
		create_enemy3c(200.0*I*afrand(0), 500, Swirl, stage5_swirl, 4+I, 70+20*afrand(1)+200.0*I, cexp(-0.05*I));
	}

	FROM_TO(700, 800, 10) {
		tsrand_fill(3);
		create_enemy3c(VIEWPORT_W+200.0*I*afrand(0), 500, Swirl, stage5_swirl, -4+afrand(1)*I, 70+20*afrand(2)+200.0*I, cexp(0.05*I));
	}

	FROM_TO(870+50*(global.diff==D_Easy), 1000, 50)
		create_enemy1c(VIEWPORT_W/4+VIEWPORT_W/2*(_i&1), 2000, BigFairy, stage5_limiter, I);

	AT(1000)
		create_enemy1c(VIEWPORT_W/2, 9000, BigFairy, stage5_laserfairy, 2.0*I);

	FROM_TO(1400+100*(D_Lunatic - global.diff), 2560, 60-5*global.diff) {
		tsrand_fill(2);
		create_enemy1c(VIEWPORT_W+200.0*I*afrand(0), 500, Swirl, stage5_miner, -3+2.0*I*afrand(1));
	}

	FROM_TO(1500, 2400, 80)
		create_enemy1c(VIEWPORT_W*(_i&1)+100.0*I, 300, Fairy, stage5_greeter, 3-6*(_i&1));

	AT(2500) {
		create_enemy1c(VIEWPORT_W/2, 2000, BigFairy, stage5_lightburst, 2.0*I);
	}

	FROM_TO(2200, 2600, 60-8*global.diff)
		create_enemy1c(VIEWPORT_W/(6+global.diff)*_i, 200, Swirl, stage5_miner, 3.0*I);

	AT(2700) {
		if(global.diff > D_Easy) {
			create_enemy1c(VIEWPORT_W-20+120*I, 2000, BigFairy, stage5_lightburst, -2.0);
		}
	}

	AT(2900)
		global.boss = stage5_spawn_midboss();

	AT(2920) {
		stage5_dialog_post_midboss();

		// TODO: determine if this hack is still required with the new dialogue system
		global.timer++;
	}

	FROM_TO(3000, 3200, 100) {
		create_enemy1c(VIEWPORT_W/2 + VIEWPORT_W/6*(1-2*(_i&1)), 2000, BigFairy, stage5_lightburst2, -1+2*(_i&1) + 2.0*I);
	}

	FROM_TO(3300, 4000, 90-10*global.diff)
		create_enemy1c(200.0*I+VIEWPORT_W*(_i&1), 1500, Fairy, stage5_superbullet, 3-6*(_i&1));

	AT(3400) {
		create_enemy2c(VIEWPORT_W/4, 6000, BigFairy, stage5_laserfairy, 2.0*I, 1);
		create_enemy2c(VIEWPORT_W/4*3, 6000, BigFairy, stage5_laserfairy, 2.0*I, 1);
	}

	FROM_TO(4200, 5000, 20-3*global.diff) {
		float f = frand();
		create_enemy3c(
			VIEWPORT_W/2+300*sin(global.frames)*cos(2*global.frames),
			400,
			Swirl,
			stage5_swirl,
			2*cexp(I*M_PI*f)+I,
			60 + 100.0*I,
			cexp(0.01*I*(1-2*(f<0.5)))
		);
	}

	FROM_TO(4320, 4400, 60) {
		double ofs = 32;
		create_enemy1c(ofs + (VIEWPORT_W-2*ofs)*(_i&1) + VIEWPORT_H*I, 200, Swirl, stage5_miner, -I);
	}

	FROM_TO(4260, 5000, 60) {
		create_enemy1c(VIEWPORT_W*(_i&1)+120*I, 400, Fairy, stage5_greeter, 6 * (1-2*(_i&1)) + I);
	}

	AT(5000) {
		create_enemy1c(VIEWPORT_W/2, 2000, BigFairy, stage5_lightburst, 2.0*I);
	}

	FROM_TO(5030, 5060, 30) {
		create_enemy1c(30.0*I+VIEWPORT_W*(_i&1), 1500, Fairy, stage5_superbullet, 2-4*(_i&1) + I);
	}

	AT(5180) {
		create_enemy1c(VIEWPORT_W/2+100, 2000, BigFairy, stage5_lightburst2, 2.0*I - 0.25);
	}

	FROM_TO(5060, 5300, 60-10*global.diff) {
		tsrand_fill(2);
		create_enemy1c(VIEWPORT_W+200.0*I*afrand(0), 500, Swirl, stage5_miner, -3+2.0*I*afrand(1));
	}

	AT(5360) {
		create_enemy1c(30.0*I+VIEWPORT_W, 1500, Fairy, stage5_superbullet, -2 + I);

	}

	AT(5390) {
		create_enemy1c(30.0*I, 1500, Fairy, stage5_superbullet, 2 + I);
	}

	AT(5500) {
		create_enemy1c(VIEWPORT_W+20 + VIEWPORT_H*0.6*I, 2000, BigFairy, stage5_lightburst, -2*I - 2);
		create_enemy1c(			 -20 + VIEWPORT_H*0.6*I, 2000, BigFairy, stage5_lightburst, -2*I + 2);
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

	FROM_TO(6100, 6350, 60-12*global.diff) {
		tsrand_fill(2);
		create_enemy1c(VIEWPORT_W+200.0*I*afrand(0), 500, Swirl, stage5_miner, -3+2.0*I*afrand(1));
	}

	AT(6300) {
		stage5_bg_lower_camera();
	}

	FROM_TO(6300, 6350, 50) {
		create_enemy1c(VIEWPORT_W/4+VIEWPORT_W/2*!(_i&1), 2000, BigFairy, stage5_limiter, 2*I);
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

