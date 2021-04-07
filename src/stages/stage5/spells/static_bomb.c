/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"

TASK(slave_ball_shot, { BoxedIkuSlave slave; }) {
	IkuSlave *slave = TASK_BIND(ARGS.slave);
	int difficulty = difficulty_value(6, 5, 4, 3);
	for(int i = 0; i < (210 / difficulty); i++) {
		PROJECTILE(
			.proto = pp_soul,
			.pos = slave->pos,
			.color = RGBA(0, 0, 1, 0),
			.move = move_asymptotic_simple(4 * cdir(0.5 * i), 3)
		);
		play_sfx("shot_special1");
		WAIT(difficulty);
	}
}

TASK(slave_laser_shot, { BoxedIkuSlave slave; int delay; }) {
	IkuSlave *slave = TASK_BIND(ARGS.slave);
	WAIT(ARGS.delay);
	int difficulty = difficulty_value(90, 80, 70, 60);
	int amount = 800 - ARGS.delay;
	for(int x = 0; x <  (amount / difficulty); x++) {
		real rand = rng_sreal();
		create_laserline(slave->pos, 10 * cnormalize(global.plr.pos - slave->pos) * cdir(0.04 * rand), 60, 120, RGBA(1, 0.3, 1, 0));
		play_sfx_delayed("laser1", 0, true, 45);
		WAIT(difficulty);
	}
}

TASK(slave_bullet_shot, { BoxedIkuSlave slave; }) {
	IkuSlave *slave = TASK_BIND(ARGS.slave);
	int difficulty = difficulty_value(5, 4, 3, 2);
	for(int x = 0; x < (520 / difficulty); x++) {
		for(int i = 1; i >= -1; i -= 2) {
			real rand = rng_real();
			PROJECTILE(
				.proto = pp_rice,
				.pos = slave->pos,
				.color = RGB(1,0,0),
				.move = move_asymptotic_simple(i * 2 * cdir(-0.3 * x + rand * I), 3)
			);
			play_sfx("shot3");
		}
		WAIT(difficulty);
	}
}

TASK(slave_explode, { BoxedIkuSlave slave; }) {
	IkuSlave *slave = TASK_BIND(ARGS.slave);

	RNG_ARRAY(ray_rand, 5);
	PARTICLE(
		.sprite = "blast_huge_rays",
		.color = color_add(RGBA(0, 0.2 + 0.5 * vrng_real(ray_rand[0]), 0.5 + 0.5 * vrng_real(ray_rand[1]), 0.0), RGBA(1, 1, 1, 0)),
		.pos = slave->pos,
		.timeout = 60 + 10 * vrng_real(ray_rand[2]),
		.draw_rule = pdraw_timeout_fade(1, 0),
		.angle = vrng_angle(ray_rand[4]),
		.layer = LAYER_PARTICLE_HIGH | 0x42,
		.flags = PFLAG_REQUIREDPARTICLE,
	);

	RNG_ARRAY(halo_rand, 5);
	PARTICLE(
		.sprite = "blast_huge_halo",
		.pos = slave->pos,
		.color = RGBA(0.3 * vrng_real(halo_rand[0]), 0.3 * vrng_real(halo_rand[1]), 1.0, 0),
		.timeout = 200 + 24 * vrng_real(halo_rand[2]),
		.draw_rule = pdraw_timeout_fade(1, 0),
		.angle = vrng_angle(halo_rand[4]),
		.layer = LAYER_PARTICLE_HIGH | 0x41,
		.flags = PFLAG_REQUIREDPARTICLE,
	);

	play_sfx("boom");
}

TASK(slave, { cmplx pos; int number; }) {
	IkuSlave *slave = stage5_midboss_slave(ARGS.pos);
	MoveParams move = move_towards(VIEWPORT_W / 4 + ARGS.number * 40 + 2.0 * I * 90 - 5 * ARGS.number * 2.0 * I, 0.03);

	INVOKE_TASK_WHEN(&slave->events.despawned, common_drop_items, &slave->pos, {
		.power = 5,
		.points = 5,
		.life = (ARGS.number == 1) ? 0 : 1
	});
	INVOKE_SUBTASK(iku_slave_move, {
		.slave = ENT_BOX(slave),
		.move = move,
	});

	INVOKE_SUBTASK_DELAYED(100, slave_ball_shot, { .slave = ENT_BOX(slave) });

	INVOKE_SUBTASK_DELAYED(200, slave_bullet_shot, { .slave = ENT_BOX(slave) });

	INVOKE_SUBTASK(slave_laser_shot, { .slave = ENT_BOX(slave), .delay = difficulty_value(500, 470, 440, 410)});

	INVOKE_SUBTASK_DELAYED(880, slave_explode, { .slave = ENT_BOX(slave) });

	WAIT(890);

	coevent_signal_once(&slave->events.despawned);
}

DEFINE_EXTERN_TASK(stage5_midboss_static_bomb) {
	STAGE_BOOKMARK(midspell1);
	Boss *b = INIT_BOSS_ATTACK(&ARGS);
	b->move = move_towards(VIEWPORT_W / 2 - VIEWPORT_H * I, 0.01);
	BEGIN_BOSS_ATTACK(&ARGS);
	WAIT(50);

	for(int x = 0; x < 3; x++) {
		INVOKE_TASK(slave, {
			.pos = b->pos,
			.number = x,
		});
		WAIT(10);
	}
}
