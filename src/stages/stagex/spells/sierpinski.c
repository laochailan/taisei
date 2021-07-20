/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "../stagex.h"
#include "stage.h"
#include "global.h"

static int idxmod(int i, int n) {   // TODO move into utils?
	return (n + i) % n;
}

attr_unused // TODO: remove me
TASK(splitter, {
	BoxedProjectile cell;
	int delay;
	cmplx v;
	Color *color;
}) {
	Projectile *cell = TASK_BIND(ARGS.cell);
	cmplx v = ARGS.v;
	Color c = *ARGS.color;

	WAIT(ARGS.delay);

#if 0
	PROJECTILE(
		.proto = pp_ball,
		.color = &c,
		.pos = cell->pos,
		.move = move_asymptotic_halflife(v, v + 2 * cabs(v), 80),
	);

	PROJECTILE(
		.proto = pp_ball,
		.color = &c,
		.pos = cell->pos,
		.move = move_asymptotic_halflife(v, v - 2 * cabs(v), 80),
	);
#else
	PROJECTILE(
		.proto = pp_ball,
		.color = &c,
		.pos = cell->pos,
		.move = move_accelerated(v, 0.05 * v),
	);
#endif

	kill_projectile(cell);
}

static Laser *create_lane_laser(cmplx origin, cmplx dir) {
	real t = 100;

	cmplx a = origin;
	cmplx b = origin + dir * hypot(VIEWPORT_W, VIEWPORT_H) * 0.6;
	cmplx m = (b - a) / t;

	Color *clr = RGBA(0, 0, 0, 0);
	Laser *l = create_laser(a, t, 1, clr, las_linear, NULL, m, 0, 0, 0);
	laser_make_static(l);
	l->width = 2;
	l->collision_active = false;
	l->ent.draw_layer = LAYER_LASER_LOW;

	return l;
}

DEFINE_EXTERN_TASK(stagex_spell_sierpinski) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	cmplx origin = VIEWPORT_W/2 + 260*I;
	boss->move = move_towards(origin, 0.02);
	BEGIN_BOSS_ATTACK(&ARGS);

	/*
	 * Rule 90 automaton with a twist
	 */

	enum { N = 65 };  // NOTE: must be odd (or it'll halt quickly)
	bool state[N] = { 0 };
	state[N/2] = 1;  // initialize with 1 bit in the middle to get a nice Sierpinski triangle pattern

	const float b = 1;
	Color colors[] = {
		{ 0.2, 0.25, 1.0, 1.0 },
		{ 1.0, 0.45, 0.1, 0.0 },
		{ 1.0, 0.65, 0.1, 0.0 },
	};
	color_mul_scalar(&colors[0], b);
	color_mul_scalar(&colors[1], b);

	DECLARE_ENT_ARRAY(Laser, lasers, N*2);
	cmplx laser_dirs[N*2];
	int laser_cooldowns[N*2] = { 0 };

	for(int i = 0; i < lasers.capacity; ++i) {
		cmplx dir = cdir((i + 0.5) / lasers.capacity * M_TAU + M_PI/2);
		laser_dirs[i] = dir;
		ENT_ARRAY_ADD(&lasers, create_lane_laser(origin, dir));
	}

	real rate = 0.04;

	for(int step = 0;; step++) {
		real pangle = carg(global.plr.pos - origin);
		log_debug("%f", pangle);

		ENT_ARRAY_FOREACH_COUNTER(&lasers, int i, Laser *l, {
			l->deathtime += 3;

			real a = carg(laser_dirs[i]);
			if(fabs(a - pangle) < 0.05 && !laser_cooldowns[i]) {
				// color_approach(&l->color, RGBA(0.2, 0.1, 0.05, 0), 0.1);
				color_lerp(&l->color, RGBA(0.05, 0.01, 0.1, 0), rate);
				if(fapproach_asymptotic_p(&l->width, 20, rate, 2) == 20) {
					create_laserline(origin, 40 * laser_dirs[i], 60, 120, RGBA(0.1, 2, 0.2, 0));
					play_sfx_delayed("laser1", 5, false, 40);
					laser_cooldowns[i] = 180;
				}
			} else {
				color_approach(&l->color, RGBA(0.3, 0.03, 0.03, 0), 0.1);
				fapproach_p(&l->width, 3, 0.5);

				if(laser_cooldowns[i]) {
					laser_cooldowns[i]--;
				}
			}
		});

		if(step % 3) {
			WAIT(1);
			continue;
		}

		bool next_state[N] = { 0 };

		for(int i = 0; i < N; ++i) {
			next_state[i] = state[idxmod(i - 1, N)] ^ state[idxmod(i + 1, N)];
		}

		int sum = 0;

		for(int i = 0; i < N; ++i) {
			if(!state[i]) {
				continue;
			}
			sum++;

			cmplx dir = cdir((i + 0.5) / N * M_TAU + M_PI/2);
			cmplx v = dir;

			attr_unused Projectile *cell = PROJECTILE(
				.proto = pp_thickrice,
				.color = &colors[0],
				.pos = origin - 100 * dir,
				.move = move_accelerated(v, 0.04 * v),
				.max_viewport_dist = VIEWPORT_W/2,
			);
			/*
			Projectile *cell2 = PROJECTILE(
				.proto = pp_flea,
				.color = &colors[1],
				.pos = boss->pos+rad*conj(dir),
				.move = {conj(v), 0, 1.006*cdir(-slip/wait)},
			);*/

			/*if(
				next_state[idxmod(i - 1, N)] ^ next_state[idxmod(i + 1, N)] ^
				     state[idxmod(i - 1, N)] ^      state[idxmod(i + 1, N)] ^
				next_state[idxmod(i - 3, N)] ^ next_state[idxmod(i + 3, N)] ^
				     state[idxmod(i - 3, N)] ^      state[idxmod(i + 3, N)]
			) {
				INVOKE_TASK(splitter, ENT_BOX(cell), 160, v, &colors[1]);
			}*/
		}

#if 0
		for(int i = 0; i < sqrt(N-sum)/6; i++) {
			cmplx souldir = cdir(M_TAU*frand());
			if(i%2==0) {
				PROJECTILE(
					.proto = frand() > 0.8 ? pp_soul : pp_bigball,
					.color = &colors[1],
					.pos = boss->pos,
					.move = move_accelerated(souldir, 0.01*I*souldir),
				);
			} else {
				PROJECTILE(
					.proto = pp_ball,
					.color = &colors[2],
					.pos = boss->pos,
					.move = move_accelerated(-souldir, 0.01*I*souldir),
				);
			}
		}
#endif

		memcpy(state, next_state, sizeof(state));
		WAIT(1);
	}
}
