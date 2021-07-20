/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "../stagex.h"
#include "global.h"
#include "common_tasks.h"

TASK(animate_radii, { real *radius_inner; real *radius_outer; }) {
	*ARGS.radius_inner = 100;
	*ARGS.radius_outer = 200;

	// TODO when animate easing from pbr_bg is somehow merged
}

TASK(pipe_conversions, { BoxedBoss bbox; BoxedProjectileArray *projs; }) {
	Boss *boss = TASK_BIND(ARGS.bbox);
	real radius_inner = 100;
	real radius_outer = 200;

	INVOKE_SUBTASK(animate_radii, &radius_inner, &radius_outer);

	for(;;) {
		ENT_ARRAY_FOREACH(ARGS.projs, Projectile *p, {
			real r = cabs(boss->pos - p->pos);
			if(r < radius_inner) {
				p->move.retention = 1;
				p->move.attraction = 0;
				projectile_set_prototype(p, pp_wave);
				p->color = *RGB(1, 1, 0);
			} else if(r < radius_outer) {
				p->move.retention = cdir(0.01);
				p->move.attraction = 0.0;

				p->move.attraction_point = global.plr.pos;
				projectile_set_prototype(p, pp_ball);
				p->color = *RGB(1, 0, 1);
			} else {
				p->move.retention = 1.01*cdir(-0.02);
				p->move.attraction = 0.0;

				projectile_set_prototype(p, pp_bigball);
				p->color = *RGB(0, 0.5, 1);
			}
		});
		YIELD;
	}

}

DEFINE_EXTERN_TASK(stagex_spell_pipe_dream) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_towards(CMPLX(VIEWPORT_W/2, VIEWPORT_H/2), 0.02);
	BEGIN_BOSS_ATTACK(&ARGS);

	int enough = 1000;
	DECLARE_ENT_ARRAY(Projectile, projs, enough);

	INVOKE_SUBTASK(pipe_conversions, ENT_BOX(boss), &projs);

	real golden = (1+sqrt(5))/2;
	for(int i = 0; ; i++) {
		ENT_ARRAY_ADD(&projs, PROJECTILE(
			.proto = pp_wave,
			.pos = boss->pos,
			.color = RGB(1, 1, 0),
			.move = move_linear(3*cdir(M_TAU/golden/golden*i)),
		));
		YIELD;
		if(i % 100 == 0) {
			ENT_ARRAY_COMPACT(&projs);
		}
	}
}
