/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "nonspells.h"
#include "../wriggle.h"

#include "global.h"
#include "common_tasks.h"

TASK(slave, { BoxedBoss boss; real rot_speed; real rot_initial; int level; }) {
	Boss *boss = TASK_BIND(ARGS.boss);
	WriggleSlave *slave = stage3_host_wriggle_slave(boss->pos);

	INVOKE_SUBTASK(wriggle_slave_follow,
		.slave = ENT_BOX(slave),
		.boss = ENT_BOX(boss),
		.rot_speed = ARGS.rot_speed,
		.rot_initial = ARGS.rot_initial
	);

	int delay = difficulty_value(9, 8, 7, 6);

	if(ARGS.level > 2)  {
		delay += 4;
	}

	WAIT(1);

	for(int t = 0;; t += WAIT(delay)) {
		play_sfx("shot1");
		cmplx aim = cnormalize(boss->pos - slave->pos);

		PROJECTILE(
			.proto = pp_rice,
			.pos = slave->pos,
			.color = RGB(0.7, 0.2, 0.1),
			.move = move_linear(3 * aim),
		);

		if(!(t % (delay * 2)) || ARGS.level > 1) {
			PROJECTILE(
				.proto = pp_thickrice,
				.pos = slave->pos,
				.color = RGB(0.7, 0.7, 0.1),
				.move = move_linear(2.5 * aim),
			);
		}

		if(ARGS.level > 2) {
			PROJECTILE(
				.proto = pp_wave,
				.pos = slave->pos,
				.color = RGB(0.3, 0.1 + 0.6 * psin(t / 25.0), 0.7),
				.move = move_linear(2 * aim),
			);
		}
	}
}

static void wriggle_nonspell_common(Boss *boss, int level) {
	int n = difficulty_value(4, 5, 6, 7);

	for(int i = 0; i < n; ++i) {
		INVOKE_SUBTASK(slave, ENT_BOX(boss),  1/70.0,  i*M_TAU/n, level);
		INVOKE_SUBTASK(slave, ENT_BOX(boss), -1/70.0, -i*M_TAU/n, level);
	}

	Rect wander_bounds = viewport_bounds(120);
	wander_bounds.bottom = 180 + 20 * level;
	real wander_dist = 60 + 10 * level;

	boss->move = move_towards(boss->pos, 0.02);

	for(;;) {
		WAIT(120);
		boss->move.attraction_point = common_wander(boss->pos, wander_dist, wander_bounds);
		WAIT(120);
	}

	STALL;
}

DEFINE_EXTERN_TASK(stage3_boss_nonspell_1) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);
	wriggle_nonspell_common(boss, 1);
}

DEFINE_EXTERN_TASK(stage3_boss_nonspell_2) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);
	wriggle_nonspell_common(boss, 2);
}

DEFINE_EXTERN_TASK(stage3_boss_nonspell_3) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);
	wriggle_nonspell_common(boss, 3);
}
