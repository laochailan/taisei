/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"
#include "../wriggle.h"

#include "common_tasks.h"
#include "global.h"

TASK(slave, { BoxedBoss boss; real rot_speed; real rot_initial; }) {
	Boss *boss = TASK_BIND(ARGS.boss);
	cmplx dir;

	WriggleSlave *slave = stage3_host_wriggle_slave(boss->pos);
	INVOKE_SUBTASK(wriggle_slave_damage_trail, ENT_BOX(slave));
	INVOKE_SUBTASK(wriggle_slave_follow,
		.slave = ENT_BOX(slave),
		.boss = ENT_BOX(boss),
		.rot_speed = ARGS.rot_speed,
		.rot_initial = ARGS.rot_initial,
		.out_dir = &dir
	);

	WAIT(300);

	for(;;WAIT(180)) {
		int cnt = 5, i;

		for(i = 0; i < cnt; ++i) {
			PROJECTILE(
				.proto = pp_ball,
				.pos = slave->pos,
				.color = RGBA(0.5, 1.0, 0.5, 0),
				.move = move_accelerated(0, 0.02 * cdir(i*M_TAU/cnt)),
			);

			if(global.diff > D_Hard) {
				PROJECTILE(
					.proto = pp_ball,
					.pos = slave->pos,
					.color = RGBA(1.0, 1.0, 0.5, 0),
					.move = move_accelerated(0, 0.01 * cdir(i*2*M_TAU/cnt)),
				);
			}
		}

		// FIXME: better sound
		play_sfx("shot_special1");
	}
}

DEFINE_EXTERN_TASK(stage3_spell_night_ignite) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_towards(VIEWPORT_W/2 + VIEWPORT_H*I/3, 0.05);
	BEGIN_BOSS_ATTACK(&ARGS);

	int nslaves = 7;

	for(int i = 0; i < nslaves; ++i) {
		INVOKE_SUBTASK(slave, ENT_BOX(boss),  1/70.0,  i*M_TAU/nslaves);
		INVOKE_SUBTASK(slave, ENT_BOX(boss), -1/70.0, -i*M_TAU/nslaves);
	}

	STALL;
}
