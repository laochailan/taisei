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

DEFINE_EXTERN_TASK(stage4_boss_nonspell_1) {
	STAGE_BOOKMARK(boss-non1);
	Boss *b = INIT_BOSS_ATTACK();
	BEGIN_BOSS_ATTACK();

	int duration = difficulty_value(300, 350, 400, 450);
	int count = difficulty_value(12, 14, 16, 18);
	int redirect_time = 40;
	
	for(;;) {

		int rice_step = difficulty_value(3, 3, 2, 2);
		WAIT(50);
		INVOKE_SUBTASK_DELAYED(10, stage4_boss_nonspell_burst, 
			.boss = ENT_BOX(b),
			.duration = duration-10,
			.count = 20
		);

		for(int i = 0; i < duration/rice_step; i++, WAIT(rice_step)) {
			play_sfx_loop("shot1_loop");
			cmplx spawn_pos = b->pos + 150 * sin(i / 8.0) + I * 100.0 * cos(i / 15.0);

			cmplx dir = cdir(M_TAU / count * i);

			cmplx redirect_vel = 1.5 * cnormalize(global.plr.pos - spawn_pos - 2 * redirect_time * dir) + 0.3 * dir;

			Projectile *p = PROJECTILE(
				.proto = pp_rice,
				.pos = spawn_pos,
				.color = RGB(1.0, 0.0, 0.5),
				.move = move_linear(2 * dir),
			);

			INVOKE_TASK_DELAYED(redirect_time, stage4_boss_nonspell_redirect,
				.proj = ENT_BOX(p),
				.new_move = move_asymptotic_simple(redirect_vel, 3)
			);
		}
	}
}
