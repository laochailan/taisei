/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"

MODERNIZE_THIS_FILE_AND_REMOVE_ME

/*static int iku_explosion(Enemy *e, int t) {
	TIMER(&t)
	AT(EVENT_KILLED) {
		spawn_items(e->pos,
			ITEM_POINTS, 5,
			ITEM_POWER, 5,
			ITEM_LIFE, creal(e->args[1])
		);

		PARTICLE(
			.sprite = "blast_huge_rays",
			.color = color_add(RGBA(0, 0.2 + 0.5 * frand(), 0.5 + 0.5 * frand(), 0.0), RGBA(1, 1, 1, 0)),
			.pos = e->pos,
			.timeout = 60 + 10 * frand(),
			.draw_rule = ScaleFade,
			.args = { 0, 0, (0 + 3*I) * (1 + 0.2 * frand()) },
			.angle = frand() * 2 * M_PI,
			.layer = LAYER_PARTICLE_HIGH | 0x42,
			.flags = PFLAG_REQUIREDPARTICLE,
		);

		PARTICLE(
			.sprite = "blast_huge_halo",
			.pos = e->pos,
			.color = RGBA(0.3 * frand(), 0.3 * frand(), 1.0, 0),
			.timeout = 200 + 24 * frand(),
			.draw_rule = ScaleFade,
			.args = { 0, 0, (0.25 + 2.5*I) * (1 + 0.2 * frand()) },
			.layer = LAYER_PARTICLE_HIGH | 0x41,
			.angle = frand() * 2 * M_PI,
			.flags = PFLAG_REQUIREDPARTICLE,
		);

		play_sound("boom");
		return 1;
	}

	FROM_TO(0, 80, 1) {
		GO_TO(e, e->pos0 + e->args[0] * 80, 0.05);
	}

	FROM_TO(90, 300, 7-global.diff) {
	}

	FROM_TO(200, 720, 6-global.diff) {
		for(int i = 1; i >= -1; i -= 2) {
			PROJECTILE(
				.proto = pp_rice,
				.pos = e->pos,
				.color = RGB(1,0,0),
				.rule = asymptotic,
				.args = { i*2*cexp(-0.3*I*_i+frand()*I), 3 }
			);
		}

		play_sound("shot3");
	}

	FROM_TO(500-30*(global.diff-D_Easy), 800, 100-10*global.diff) {
		play_sfx_delayed("laser1", 0, true, 45);
	}

	return 1;
}*/

TASK(slave, { cmplx pos; int number; }) {
	IkuSlave *slave = stage5_midboss_slave(ARGS.pos);
	MoveParams move = move_towards(0, 0.03);
	move.attraction_point = VIEWPORT_W / 2 + 2.0 * I * 100;

	INVOKE_SUBTASK(iku_slave_move, {
		.slave = ENT_BOX(slave),
		.move = move,
	});

	for(int i = 0; i < 210; i++) {
		if(i % 5 == 0) {
			PROJECTILE(
				.proto = pp_soul,
				.pos = slave->pos,
				.color = RGBA(0, 0, 1, 0),
				.rule = asymptotic,
				.args = { 4 * cdir(0.5 * i), 3 }
			);
			play_sfx("shot_special1");
		}
		WAIT(1);
	}

	for(int x = 0; x < 300; x++) {
		if(x % 50 == 0) {
			create_laserline(slave->pos, 10*cexp(I*carg(global.plr.pos-slave->pos)+0.04*I*(1-2*frand())), 60, 120, RGBA(1, 0.3, 1, 0));
			play_sfx_delayed("laser1", 0, true, 45);
		}
		WAIT(1);
	}
}

DEFINE_EXTERN_TASK(stage5_midboss_iku_explosion) {
	STAGE_BOOKMARK(spell1);

	Boss *b = INIT_BOSS_ATTACK(&ARGS);
	// iku moves
	b->move = move_towards(VIEWPORT_W / 2 - VIEWPORT_H * I, 0.01);
	WAIT(100);

//	create_enemy3c(b->pos, ENEMY_IMMUNE, iku_slave_visual, iku_explosion, -2 - 0.5 * x + I * x, x == 1, 1);
	for(int x = 0; x < 3; x++) {
		INVOKE_TASK(slave, {
			.pos = b->pos,
			.number = x,
		});
	}

	WAIT(960);
	enemy_kill_all(&global.enemies);
}
