/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "nonspells.h"

TASK(plainball_shoot, { BoxedBoss boss; }) {
	Boss *boss = TASK_BIND(ARGS.boss);

	for(int x = 0;; x++, WAIT(difficulty_value(4, 3, 2, 1))) {
		if(rng_chance(0.9)) {
			PROJECTILE(
				.proto = pp_plainball,
				.pos = boss->pos,
				.color = RGB(0.2, 0, 0.8),
				.move = move_linear(cdir(0.1 * x)),
			);
		}
	}
}

TASK(ball_shoot, { BoxedBoss boss; }) {
	Boss *boss = TASK_BIND(ARGS.boss);

	for(int x = 0;; x++, WAIT(60)) {
		aniplayer_queue(&boss->ani, (x&1) ? "dashdown_left" : "dashdown_right",1);
		aniplayer_queue(&boss->ani, "main", 0);

		int count = difficulty_value(11, 12, 13, 14);
		cmplx n = cdir(0.1 * rng_sreal()) * cnormalize(global.plr.pos - boss->pos);
		for(int i = 0; i < count; i++) {
			PROJECTILE(
				.proto = pp_ball,
				.pos = boss->pos,
				.color = RGBA(0.4, 1.0, 1.0, 0.0),
				.move = move_asymptotic_simple((i + 2) * 0.4 * n + 0.2 * difficulty_value(0, 1, 2, 3) * rng_real(), 3),
			);
		}
		play_sfx("shot2");
		play_sfx("redirect");
	}
}

DEFINE_EXTERN_TASK(stage5_boss_nonspell_3) {
	STAGE_BOOKMARK(boss-non3);
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_towards(VIEWPORT_W/2 + 200.0 * I, 0.06);
	BEGIN_BOSS_ATTACK(&ARGS);

	INVOKE_SUBTASK(iku_spawn_clouds);
	INVOKE_SUBTASK(plainball_shoot, ENT_BOX(boss));
	INVOKE_SUBTASK_DELAYED(60, ball_shoot, ENT_BOX(boss));

	WAIT(50);
	boss->move.attraction = 0.02;
	for(;;) {
		boss->move.attraction_point = 100 + 200.0 * I;
		WAIT(200);
		boss->move.attraction_point = VIEWPORT_W-100 + 200.0 * I;
		WAIT(200);
	}
}
