/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "nonspells.h"

TASK(boss_move, { BoxedBoss boss; }) {
	Boss *boss = TASK_BIND(ARGS.boss);
	for(;;) {
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
	BEGIN_BOSS_ATTACK(&ARGS);

	INVOKE_SUBTASK(iku_spawn_clouds);
	INVOKE_SUBTASK(boss_move, { .boss = ENT_BOX(boss) });

	int offset = difficulty_value(0, 1, 2, 3);
	int count = difficulty_value(11, 12, 13, 14);
	for(int x = 0;; x += WAIT(50)) {
		for(int i = 0; i < count; i++) {
			PROJECTILE(
				.proto = pp_ball,
				.pos = boss->pos,
				.color = RGBA(0.4, 1.0, 1.0, 0),
				.move = move_asymptotic_simple((i + 2) * 0.4 * cnormalize(global.plr.pos - boss->pos) + 0.2 * offset * rng_sreal(), 3),
			);
		}
		play_sfx("shot2");
		play_sfx("redirect");
	}
}

