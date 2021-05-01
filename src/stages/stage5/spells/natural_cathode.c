/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"

static cmplx cathode_laser(Laser *l, float t) {
	if(t == EVENT_BIRTH) {
		l->shader = res_shader_optional("lasers/iku_cathode");
		return 0;
	}

	l->args[1] = I * cimag(l->args[1]);

	return l->pos + l->args[0] * t * cexp(l->args[1] * t);
}

DEFINE_EXTERN_TASK(stage5_spell_natural_cathode) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_towards(VIEWPORT_W/2 + 200.0 * I, 0.06);
	BEGIN_BOSS_ATTACK(&ARGS);

	for(int x = 0;; x++, WAIT(difficulty_value(60, 50, 40, 30))) {
		aniplayer_hard_switch(&boss->ani, (x & 1) ? "dashdown_left" : "dashdown_right", 1);
		aniplayer_queue(&boss->ani, "main", 0);

		int i;
		int c = 7 + global.diff/2;

		double speedmod = 1 - 0.3 * difficulty_value(0, 0, 0, 1);
		for(i = 0; i < c; i++) {
			Projectile *p = PROJECTILE(
				.proto = pp_bigball,
				.pos = boss->pos,
				.color = RGBA(0.2, 0.4, 1.0, 0.0),
			);

			INVOKE_TASK(iku_induction_bullet, {
				.p = ENT_BOX(p),
				.radial_vel = speedmod * 2 * cdir(M_TAU * rng_real()),
				.angular_vel = speedmod * 0.01 * I * (1 - 2 * (x & 1)),
				.mode = 1,
			});

			if(i < c * 3/4) {
				create_lasercurve2c(boss->pos, 60, 200, RGBA(0.4, 1, 1, 0), cathode_laser, 2 * cdir(M_TAU * M_PI * rng_real()), 0.015 * I * (1 - 2 * (x & 1)));
			}
		}

		// TODO: select a better set of sounds someday
		play_sfx("shot_special1");
		play_sfx("redirect");
		play_sfx("shot3");
		play_sfx("shot2");
	}
}
