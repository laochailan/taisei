/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "nonspells.h"

static cmplx bolts2_laser(Laser *l, float t) {
	if(t == EVENT_BIRTH) {
		l->shader = res_shader_optional("lasers/iku_lightning");
		return 0;
	}

	double diff = creal(l->args[2]);
	return creal(l->args[0]) + I * cimag(l->pos) + sign(cimag(l->args[0] - l->pos)) * 0.06 * I * t * t + (20 + 4 * diff) * sin(t * 0.025 * diff + creal(l->args[0])) * l->args[1];
}

TASK(laser_drop, { BoxedBoss boss; }) {
	Boss *boss = TASK_BIND(ARGS.boss);
	for(int x = 0;; x++, WAIT(60)) {
		aniplayer_queue(&boss->ani, (x&1) ? "dashdown_left" : "dashdown_right", 1);
		aniplayer_queue(&boss->ani, "main", 0);
		create_lasercurve3c(creal(global.plr.pos), 100, 200, RGBA(0.3, 1, 1, 0), bolts2_laser, global.plr.pos, (x&1) * 2 - 1, global.diff);
		play_sfx_ex("laser1", 0, false);
	}
}

TASK(ball_shoot, { BoxedBoss boss; }) {
	Boss *boss = TASK_BIND(ARGS.boss);

	for(int x = 0;; x++, WAIT(difficulty_value(4, 3, 2, 1))) {
		if(rng_chance(0.9)) {
			PROJECTILE(
				.proto = pp_plainball,
				.pos = boss->pos,
				.color = RGB(0.2, 0, 0.8),
				.move = move_linear(cdir(0.1 * x))
			);
		}
		play_sfx_loop("shot1_loop");
	}
}

DEFINE_EXTERN_TASK(stage5_boss_nonspell_2) {
	STAGE_BOOKMARK(boss-non2);
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_towards(VIEWPORT_W/2 + 200.0 * I, 0.06);
	BEGIN_BOSS_ATTACK(&ARGS);

	INVOKE_SUBTASK(iku_spawn_clouds);
	INVOKE_SUBTASK(ball_shoot, ENT_BOX(boss));
	INVOKE_SUBTASK(laser_drop, ENT_BOX(boss));

	WAIT(50);
	boss->move.attraction = 0.02;
	for(;;) {
		boss->move.attraction_point = 100 + 200.0 * I;
		WAIT(200);
		boss->move.attraction_point = VIEWPORT_W-100 + 200.0 * I;
		WAIT(200);
	}
}
