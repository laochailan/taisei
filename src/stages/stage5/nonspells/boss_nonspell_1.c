/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "nonspells.h"

TASK(spawn_clouds, NO_ARGS) {
	for(int i = 0;; i += WAIT(2)) {
		iku_nonspell_spawn_cloud();
	}
}

TASK(boss_move, { BoxedBoss boss; }) {
	Boss *boss = TASK_BIND(ARGS.boss);
	for(;; YIELD) {
		boss->move = move_towards(100 + 300.0 * I, 0.02);
		WAIT(100);
		boss->move = move_towards(VIEWPORT_W/2 + 100.0 * I, 0.02);
		WAIT(100);
		boss->move = move_towards(VIEWPORT_W - 100 + 300.0 * I, 0.02);
		WAIT(100);
		boss->move = move_towards(VIEWPORT_W/2 + 100.0 * I, 0.02);
	}
}

DEFINE_EXTERN_TASK(stage5_boss_nonspell_1) {
	STAGE_BOOKMARK(nonspell1);
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);

	INVOKE_SUBTASK(spawn_clouds);
	INVOKE_SUBTASK(boss_move, { .boss = ENT_BOX(boss) });

	for(int x = 0;; x += WAIT(50)) {
		int i, c = 10+global.diff;
		for(i = 0; i < c; i++) {
			PROJECTILE(
				.proto = pp_ball,
				.pos = boss->pos,
				.color = RGBA(0.4, 1.0, 1.0, 0),
				.move = move_asymptotic_simple((i + 2) * 0.4 * cnormalize(global.plr.pos - boss->pos) + 0.2 * (global.diff - 1) * rng_sreal(), 3),
			);
		}

		play_sfx("shot2");
		play_sfx("redirect");
	}

}

