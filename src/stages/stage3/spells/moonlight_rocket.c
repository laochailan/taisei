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

TASK(rocket_slave, { BoxedBoss boss; real rot_speed; real rot_initial; }) {
	Boss *boss = TASK_BIND(ARGS.boss);
	WriggleSlave *slave = stage3_host_wriggle_slave(boss->pos);
	INVOKE_TASK(wriggle_slave_damage_trail, ENT_BOX(slave));
	INVOKE_TASK(wriggle_slave_follow, ENT_BOX(slave), ENT_BOX(boss), ARGS.rot_speed, ARGS.rot_initial);
	STALL;
}

DEFINE_EXTERN_TASK(stage3_spell_moonlight_rocket) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_towards(VIEWPORT_W/2 + VIEWPORT_H*I/2.5, 0.05);
	BEGIN_BOSS_ATTACK(&ARGS);

	int nslaves = difficulty_value(2, 3, 4, 5);

	for(int i = 0; i < nslaves; ++i) {
		for(int s = -1; s < 2; s += 2) {
			INVOKE_SUBTASK(rocket_slave, ENT_BOX(boss), s/70.0, s*i*M_TAU/nslaves);
		}
	}

	STALL;
}
