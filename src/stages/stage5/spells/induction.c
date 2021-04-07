/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"

DEFINE_EXTERN_TASK(stage5_spell_induction) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	// thwart safespots
 	cmplx ofs = global.diff > D_Normal ? 10 * I : 0;
	boss->move = move_towards(VIEWPORT_W/2 + 200.0 * I + ofs, 0.06);
	BEGIN_BOSS_ATTACK(&ARGS);

	aniplayer_queue(&boss->ani, "dashdown_wait", 0);

	for(int x = 0;; x++, WAIT(8)) {
		play_sfx("redirect");
		play_sfx("shot1_loop");

		int c = 6;
		int c2 = 6 - (global.diff/4);
		for(int i = 0; i < c; i++) {
			for(int j = 0; j < 2; j++) {
				Color *clr = RGBA(1 - 1 / (1 + 0.1 * (x % c2)), 0.5 - 0.1 * (x % c2), 1.0, 0.0);
				float shift = 0.6 * (x / c2);
				float a = -0.0002 * difficulty_value(0, 1, 2, 3);
				if(global.diff >= D_Hard) {
					a += 0.0005;
				}

				Projectile *p = PROJECTILE(
					.proto = pp_ball,
					.pos = boss->pos,
					.color = clr,
					.max_viewport_dist = 400 * difficulty_value(0, 0, 1, 1),
				);

				INVOKE_TASK(iku_induction_bullet, {
					.p = ENT_BOX(p),
					.m1 = 2 * cdir(M_TAU / c * i + M_PI/2 + shift),
					.m2 = (0.01 + 0.001 * difficulty_value(1, 2, 3, 4)) * I * (1 - 2 * j) + a,
				});
			}
		}
	}
}
