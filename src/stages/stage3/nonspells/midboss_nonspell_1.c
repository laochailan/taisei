/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "nonspells.h"
#include "../scuttle.h"

#include "global.h"

TASK(midboss_delaymove, { BoxedBoss boss; } ) {
	Boss *boss = TASK_BIND(ARGS.boss);

	boss->move.attraction_point = 5*VIEWPORT_W/6 + 200*I;
	boss->move.attraction = 0.001;
}

DEFINE_EXTERN_TASK(stage3_midboss_nonspell_1) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);

	int difficulty = difficulty_value(1, 2, 3, 4);
	int intensity = difficulty_value(18, 19, 20, 21);
	int velocity_intensity = difficulty_value(2, 3, 4, 5);
	INVOKE_SUBTASK_DELAYED(400, midboss_delaymove, ENT_BOX(boss));

	for(;;) {
		DECLARE_ENT_ARRAY(Projectile, projs, intensity*4);

		// fly through Scuttle, wind up on other side in a starburst pattern
		for(int i = 0; i < intensity; ++i) {
			cmplx v = (2 - psin((fmax(3, velocity_intensity) * 2 * M_PI * i / (float)intensity) + i)) * cdir(2 * M_PI / intensity * i);
			ENT_ARRAY_ADD(&projs,
				PROJECTILE(
					.proto = pp_wave,
					.pos = boss->pos - v * 50,
					.color = i % 2 ? RGB(0.7, 0.3, 0.0) : RGB(0.3, .7, 0.0),
					.move = move_asymptotic_simple(v, 2.0)
				)
			);
		}

		WAIT(80);

		// halt acceleration for a moment
		ENT_ARRAY_FOREACH(&projs, Projectile *p, {
			p->move.acceleration = 0;
		});

		WAIT(30);

		// change direction, fly towards player
		ENT_ARRAY_FOREACH(&projs, Projectile *p, {
			int count = 6;

			// when the shot "releases", add a bunch of particles and some extra bullets
			for(int i = 0; i < count; ++i) {
				PARTICLE(
					.sprite = "smoothdot",
					.pos = p->pos,
					.color = RGBA(0.8, 0.6, 0.6, 0),
					.timeout = 100,
					.draw_rule = pdraw_timeout_scalefade(0, 0.8, 1, 0),
					.move = move_asymptotic_simple(p->move.velocity + rng_dir(), 0.1)
				);

				real offset = global.frames/15.0;
				if(global.diff > D_Hard && global.boss) {
					offset = M_PI + carg(global.plr.pos - global.boss->pos);
				}

				PROJECTILE(
					.proto = pp_thickrice,
					.pos = p->pos,
					.color = RGB(0.4, 0.3, 1.0),
					.move = move_linear(-cdir(((i * 2 * M_PI/count + offset)) * (1.0 + (difficulty > 2))))
				);
			}
			spawn_projectile_highlight_effect(p);
			p->move = move_linear((3 + (2.0 * difficulty) / 4.0) * (cnormalize(global.plr.pos - p->pos)));

		});
	}
}
