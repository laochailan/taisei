/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "nonspells.h"
#include "../kurumi.h"

#include "global.h"

DEFINE_EXTERN_TASK(stage4_boss_nonspell_2) {
	STAGE_BOOKMARK(boss-non2);
	Boss *b = INIT_BOSS_ATTACK();
	BEGIN_BOSS_ATTACK();

	int duration = 350;

	int count = difficulty_value(10, 12, 14, 16);
	int rice_step = difficulty_value(43, 36, 29, 22);
	int redirect_time = 20;

	int time = 0;
	for(;;) {
		WAIT(50);

		INVOKE_SUBTASK_DELAYED(10, stage4_boss_nonspell_burst, 
			.boss = ENT_BOX(b),
			.duration = duration-10,
			.count = 20
		);

		for(int i = 0; i < duration/rice_step; i++, WAIT(rice_step)) {	
			cmplx boss_target = VIEWPORT_W / 2.0 + VIEWPORT_W / 3.0 * sin(time / 220.0) + I*cimag(b->pos);

			b->move = move_towards(boss_target, 0.02);
			
			play_sfx("shot1");
			cmplx spawn_pos = b->pos + 150 * sin(i) + 100.0 * I * cos(i);

			for(int j = 0; j < count; j++) {
				cmplx dir = cdir(M_TAU / count * j);
				Projectile *p = PROJECTILE(
					.proto = pp_rice,
					.pos = spawn_pos,
					.color = RGB(1,0,0.5),
					.move = move_linear(3 * dir)
				);
				cmplx redirect_vel = 1.5 * cnormalize(global.plr.pos - spawn_pos - 2 * redirect_time * dir) + 0.4  * dir;

				INVOKE_TASK_DELAYED(redirect_time, stage4_boss_nonspell_redirect,
					.proj = ENT_BOX(p),
					.new_move = move_asymptotic_simple(redirect_vel, 3)
				);

			}
			time++;
		}
	}
}
