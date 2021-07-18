/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"

TASK(cloud_shoot, NO_ARGS) {
	for(int x = 0;; x += WAIT(difficulty_value(6, 5, 4, 3))) {
		if(global.diff >= D_Hard) {
			PROJECTILE(
				.proto = pp_thickrice,
				.pos = VIEWPORT_W * rng_real(),
				.color = RGB(0, 0.3, 0.7),
				.move = move_accelerated(0, 0.01 * I),
			);
		} else {
			PROJECTILE(
				.proto = pp_rice,
				.pos = VIEWPORT_W * rng_real(),
				.color = RGB(0, 0.3, 0.7),
				.move = move_linear(2 * I),
			);
		}
	}
}

TASK(ball_shoot, NO_ARGS) {
	int shot_angle = difficulty_value(140, 160, 180, 200);
	int shot_count = difficulty_value(7, 8, 9, 10);
	int shot_accel = difficulty_value(5, 6, 7, 8);
	for(int x = 0;; x += WAIT(difficulty_value(19, 17, 15, 13))) {
		RNG_ARRAY(rand, 4);
		cmplx p1 = VIEWPORT_W * vrng_real(rand[0]) + VIEWPORT_H/2 * I * vrng_real(rand[1]);
		cmplx p2 = p1 + shot_angle * cdir(0.5 * vrng_sreal(rand[2])) * (1 - 2 * (vrng_chance(rand[3], 0.5)));

		for(int i = -shot_count * 0.5; i <= shot_count * 0.5; i++) {
			PROJECTILE(
				.proto = pp_ball,
				.pos = p1 + (p2 - p1) / shot_count * i,
				.color = RGBA(1 - 1 / (1 + fabs(0.1 * i)), 0.5 - 0.1 * abs(i), 1, 0),
				.move = move_accelerated(0, (0.001 * shot_accel) * cnormalize(p2 - p1) * cdir(M_PI/2 + 0.2 * i)),
			);
		}
		play_sfx("shot_special1");
		play_sfx("redirect");
	}
}

DEFINE_EXTERN_TASK(stage5_spell_atmospheric_discharge) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_towards(VIEWPORT_W/2 + 200.0 * I, 0.06);
	BEGIN_BOSS_ATTACK(&ARGS);

	Rect wander_bounds = viewport_bounds(64);
	wander_bounds.top += 64;
	wander_bounds.bottom = VIEWPORT_H * 0.4;

	INVOKE_SUBTASK(ball_shoot);
	INVOKE_SUBTASK(cloud_shoot);

	boss->move.attraction = 0.03;
	for(;;WAIT(100)) {
		boss->move.attraction_point = common_wander(boss->pos, VIEWPORT_W * 0.3, wander_bounds);
	}
}
