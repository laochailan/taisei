/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "coroutine.h"
#include "taisei.h"

#include "timeline.h"
#include "yumemi.h"
#include "stagex.h"
#include "draw.h"
#include "nonspells/nonspells.h"

#include "stage.h"
#include "global.h"
#include "common_tasks.h"
#include "enemy_classes.h"

static void stagex_dialog_post_boss(void) {
	PlayerMode *pm = global.plr.mode;
	INVOKE_TASK_INDIRECT(StageExPostBossDialog, pm->dialog->StageExPostBoss);
}

TASK(glider_bullet, {
	cmplx pos; real dir; real spacing; int interval;
}) {
	const int nproj = 5;
	const int nstep = 4;
	BoxedProjectile projs[nproj];

	const cmplx positions[][5] = {
		{1+I, -1, 1, -I, 1-I},
		{2, I, 1, -I, 1-I},
		{2, 1+I, 2-I, -I, 1-I},
		{2, 0, 2-I, 1-I, 1-2*I},
	};

	cmplx trans = cdir(ARGS.dir+1*M_PI/4)*ARGS.spacing;

	for(int i = 0; i < nproj; i++) {
		projs[i] = ENT_BOX(PROJECTILE(
			.pos = ARGS.pos+positions[0][i]*trans,
			.proto = pp_ball,
			.color = RGBA(0,0,1,1),
		));
	}

	for(int step = 0;; step++) {
		int cur_step = step%nstep;
		int next_step = (step+1)%nstep;

		int dead_count = 0;
		for(int i = 0; i < nproj; i++) {
			Projectile *p = ENT_UNBOX(projs[i]);
			if(p == NULL) {
				dead_count++;
			} else {
				p->move.retention = 1;
				p->move.velocity = -(positions[cur_step][i]-(1-I)*(cur_step==3)-positions[next_step][i])*trans/ARGS.interval;
			}
		}
		if(dead_count == nproj) {
			return;
		}
		WAIT(ARGS.interval);
	}
}

TASK(glider_fairy, {
	cmplx pos; cmplx dir;
}) {
	Enemy *e = TASK_BIND(espawn_big_fairy(VIEWPORT_W/2-10*I, ITEMS(
		.power = 3,
		.points = 5
	)));

	YIELD;

	for(int i = 0; i < 80; ++i) {
		e->pos += cnormalize(ARGS.pos-e->pos)*2;
		YIELD;
	}

	for(int i = 0; i < 3; i++) {
		real aim = carg(global.plr.pos - e->pos);
		INVOKE_TASK(glider_bullet, e->pos, aim-0.7, 20, 6);
		INVOKE_TASK(glider_bullet, e->pos, aim, 25, 3);
		INVOKE_TASK(glider_bullet, e->pos, aim+0.7, 20, 6);
		WAIT(80-20*i);
	}

	for(;;) {
		e->pos += 2*(creal(e->pos)-VIEWPORT_W/2 > 0)-1;
		YIELD;
	}
}

TASK(aimgrind_fairy, {
	cmplx pos;
}) {
	Enemy *e = TASK_BIND(espawn_big_fairy(ARGS.pos, NULL));
	cmplx v = CMPLX(1-2*(creal(ARGS.pos)<VIEWPORT_W/2), 1);

	for(int i = 0; i < 30; i++) {
		for(int k = 0; k < 2; k++) {
			cmplx d = global.plr.pos-e->pos;
			cmplx aim = cnormalize(d);
			real r = cabs(d)*(0.6-0.2*k);
			real v0 = 6;
			real phi = acos(1-0.5*v0*v0/r/r);
			for(int j = -1; j <= 1; j+=2) {
				PROJECTILE(
					.pos = e->pos,
					.proto = pp_bullet,
					.color = RGBA(0.1,0.4,1,1),
					.move = {-v0*j*aim*I,0,cdir(phi*j)},
					.timeout = M_PI/phi,
				);
			}
			PROJECTILE(
				.pos = e->pos,
				.proto = pp_ball,
				.color = RGBA(0.8,0.1,0.5,1),
				.move = move_asymptotic(0, aim*5*cdir(0.3*sin(i)),0.1),
			);
		}
		WAIT(10);

	}
	e->move = move_linear(v);

}

TASK(rocket_proj, { cmplx pos; cmplx dir; }) {
	Projectile *p = TASK_BIND(PROJECTILE(
		.pos = ARGS.pos,
		.proto = pp_bigball,
		.color = RGBA(0,0.2,1,0.0),
		.move = move_accelerated(0,0.2*ARGS.dir)
	));
	real phase = rng_angle();
	real period = rng_range(0.2,0.5);
	for(int i = 0; ; i++) {
		PROJECTILE(
			.pos = p->pos,
			.proto = pp_bullet,
			.color = RGBA(0.1,0.2,0.9,1.0),
			.move = move_accelerated(0,0.01*cdir(period*i+phase)),
		);
		YIELD;
	}
}


TASK(rocket_fairy, { cmplx pos; }) {
	Enemy *e = TASK_BIND(espawn_fairy_red(ARGS.pos, NULL));

	e->move = move_linear(0.5*I);

	cmplx aim = rng_dir();
	int rockets = 3;
	for(int i = 0; i < rockets; i++) {
		INVOKE_TASK(rocket_proj, e->pos, aim*cdir(M_TAU/rockets*i));
		WAIT(10);
	}
}

TASK(ngoner_proj, { cmplx pos; cmplx target; int stop_time; int laser_time; cmplx laser_vel; }) {
	Projectile *p = TASK_BIND(PROJECTILE(
		.pos = ARGS.pos,
		.proto = pp_bullet,
		.color = RGBA(0,0.2,1,0.0),
		.move = move_linear(ARGS.target/ARGS.stop_time)
	));
	WAIT(ARGS.stop_time);
	p->move = move_stop(0.5);

	WAIT(ARGS.laser_time-ARGS.stop_time);
	p->move = move_linear(ARGS.laser_vel);

	PROJECTILE(
		.pos = p->pos,
		.proto = pp_ball,
		.color = RGBA(0.1,0.9,1,0.0),
		.move = move_linear(2*cnormalize(ARGS.target)),
	);
}

TASK(ngoner_laser, { cmplx pos; cmplx dir; }) {
	create_laser(ARGS.pos, 60, 360, RGBA(0.1,0.9,1.0,0.1), las_linear, 0, ARGS.dir, 0, 0, 0);
	create_laser(ARGS.pos, 60, 360, RGBA(0.1,0.9,1.0,0.1), las_linear, 0, -ARGS.dir, 0, 0, 0);
}

TASK(ngoner_fairy, { cmplx pos; }) {
	Enemy *e = TASK_BIND(espawn_fairy_red(ARGS.pos, NULL));

	cmplx rot = rng_dir();

	int corners = 6;
	int projs_per_site = 11;
	int assembly_time = 10;
	real site_length = 70;

	real b = site_length / 2 * tan(M_PI* (0.5 - 1.0 / corners));

	int laser_time = corners*projs_per_site + assembly_time + 10;

	for(int i = 0; i < corners; i++) {
		cmplx offset = rot*b*cdir(M_TAU/corners*i);
		INVOKE_TASK_DELAYED(laser_time, ngoner_laser, e->pos + offset, 5*I*cnormalize(offset));
	}

	for(int s = 0; s < projs_per_site; s++) {
		for(int i = 0; i < corners; i++) {
			real phase = M_TAU/corners*(s/(real)projs_per_site-0.5);
			real radius = b/cos(phase);
			cmplx target = rot*radius*cdir(phase+M_TAU/corners*i);

			int laser_delay = laser_time + 2*abs(s-projs_per_site/2) - i - s*corners;
			cmplx laser_vel = rot*3.5*cdir(M_TAU/corners*i+0.06*(s-projs_per_site/2.0));


			INVOKE_TASK(ngoner_proj, e->pos, target, assembly_time, laser_delay, laser_vel);
			YIELD;
		}
	}
	e->move = move_linear(I);
}

static Boss *stagex_spawn_scuttle(cmplx pos0) {
	Boss *scuttle = create_boss("Scutƫle", "scuttle", pos0);
	boss_set_portrait(scuttle, "scuttle", NULL, "normal");
	scuttle->shadowcolor = *RGBA(0.5, 0.0, 0.22, 1);
	scuttle->glowcolor = *RGBA(0.30, 0.0, 0.12, 0);

	return scuttle;
}

TASK(scuttle_appear, { cmplx pos; }) {
	STAGE_BOOKMARK(midboss);
	Boss *boss = global.boss = TASK_BIND(stagex_spawn_scuttle(ARGS.pos));

	boss_add_attack_from_info(boss, &stagex_spells.midboss.trap_representation, false);
	boss_add_attack_from_info(boss, &stagex_spells.midboss.fork_bomb, false);
	boss_engage(global.boss);
}

TASK(scuttleproj_appear, NO_ARGS) {
	Projectile *p = TASK_BIND(PROJECTILE(
		.pos = VIEWPORT_W/2,
		.proto = pp_soul,
		.color = RGBA(0,0.2,1,0),
		.move = move_towards(global.plr.pos, 0.015),
		.flags = PFLAG_NOCLEAR | PFLAG_NOCOLLISION | PFLAG_NOAUTOREMOVE,
	));
	WAIT(20);

	int num_spots = 32;
	for(int i = 0; i < 400; i++) {
		int spot = rng_range(0,num_spots);
		cmplx offset = cdir(M_TAU/num_spots*spot);
		real clr = rng_range(0,1);

		cmplx vel = 2*rng_dir();
		PROJECTILE(
			.pos = p->pos + 50*offset,
			.proto = pp_bullet,
			.color = RGBA(clr,0.2,1,0),
			.flags = PFLAG_MANUALANGLE,
			.angle = carg(offset),
			.move = move_linear(vel),
			.timeout = rng_range(20,60),
		);
		YIELD;
		p->move.attraction_point = global.plr.pos;

		if(i % 5 == 0) {
			if(rng_chance(0.2)) {
				p->sprite = res_sprite("proj/bigball");
				p->pos += 30*rng_dir();
			} else {
				p->sprite = res_sprite("proj/soul");
			}
		}
	}
	kill_projectile(p);

	INVOKE_TASK(scuttle_appear, p->pos);
}

TASK(yumemi_appear, { BoxedBoss boss; }) {
	Boss *boss = TASK_BIND(ARGS.boss);
	boss->move = move_towards(VIEWPORT_W/2 + 180*I, 0.015);
}

TASK(spawn_boss, NO_ARGS) {
	STAGE_BOOKMARK(boss);

	Boss *boss = global.boss = stagex_spawn_yumemi(5*VIEWPORT_W/4 - 200*I);
	PlayerMode *pm = global.plr.mode;

	Attack *opening_attack = boss_add_attack(boss, AT_Normal, "Opening", 60, 40000, NULL, NULL);
	StageExPreBossDialogEvents *e;

	INVOKE_TASK_INDIRECT(StageExPreBossDialog, pm->dialog->StageExPreBoss, &e);
	INVOKE_TASK_WHEN(&e->boss_appears, yumemi_appear, ENT_BOX(boss));
	INVOKE_TASK_WHEN(&e->music_changes, stagex_boss_nonspell_1, ENT_BOX(boss), opening_attack);
	INVOKE_TASK_WHEN(&e->music_changes, common_start_bgm, "stagexboss");
	WAIT_EVENT(&global.dialog->events.fadeout_began);

	boss_add_attack_from_info(boss, &stagex_spells.boss.sierpinski, false);
 	boss_add_attack_task(boss, AT_Normal, "non2", 60, 40000, TASK_INDIRECT(BossAttack, stagex_boss_nonspell_2), NULL);
	boss_add_attack_from_info(boss, &stagex_spells.boss.infinity_network, false);
	boss_add_attack_task(boss, AT_Normal, "non3", 60, 40000, TASK_INDIRECT(BossAttack, stagex_boss_nonspell_3), NULL);
	boss_add_attack_from_info(boss, &stagex_spells.boss.mem_copy, false);
	boss_add_attack_task(boss, AT_Normal, "non4", 60, 40000, TASK_INDIRECT(BossAttack, stagex_boss_nonspell_4), NULL);
	boss_add_attack_from_info(boss, &stagex_spells.boss.pipe_dream, false);
// 	boss_add_attack_task(boss, AT_Normal, "non5", 60, 40000, TASK_INDIRECT(BossAttack, stagex_boss_nonspell_5), NULL);
//	boss_add_attack_task(boss, AT_Normal, "non6", 60, 40000, TASK_INDIRECT(BossAttack, stagex_boss_nonspell_6), NULL);

	boss_engage(boss);
}

DEFINE_EXTERN_TASK(stagex_timeline) {
	for(int i = 0; i < 20; i++) {
		real rx = rng_range(-1,1)*100;
		real ry = rng_range(-1,1)*50;
		INVOKE_TASK_DELAYED(400+i*50, rocket_fairy, CMPLX(VIEWPORT_W*0.5+rx, VIEWPORT_H*0.3+ry));
	}
	for(int i = 0; i < 20; i++) {
		real rx = rng_range(-1,1)*100;
		real ry = rng_range(-1,1)*50;
		INVOKE_TASK_DELAYED(1000+i*100, aimgrind_fairy, CMPLX(VIEWPORT_W*0.5+rx, VIEWPORT_H*0.3+ry));
	}
	for(int i = 0; i < 20; i++) {
		real rx = rng_range(-1,1)*100;
		real ry = rng_range(-1,1)*50;
		INVOKE_TASK_DELAYED(1500+i*70, ngoner_fairy, CMPLX(VIEWPORT_W*0.5+rx, VIEWPORT_H*0.3+ry));
	}
	for(int i = 0; i < 4;i++) {
		INVOKE_TASK_DELAYED(2000+i*100, glider_fairy, CMPLX(VIEWPORT_W*(i&1), VIEWPORT_H*0.5), 3*I);
		WAIT(140);
	}

	INVOKE_TASK_DELAYED(2500, scuttleproj_appear);
	while(!global.boss) YIELD;
	WAIT_EVENT(&global.boss->events.defeated);

	WAIT(1000);

	stagex_get_draw_data()->tower_global_dissolution = 1;
	INVOKE_TASK(spawn_boss);
	while(!global.boss) YIELD;
	WAIT_EVENT(&global.boss->events.defeated);

	stage_unlock_bgm("stagexboss");

	WAIT(240);
	stagex_dialog_post_boss();
	WAIT_EVENT(&global.dialog->events.fadeout_began);

	WAIT(5);
	stage_finish(GAMEOVER_SCORESCREEN);

}
