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

static int polkadot_circle_approx_size_upper_bound(int subdiv) {
	// conservative estimate based on circle area formula
	// actual number of elements will be lower
	return ceil(M_PI * pow((subdiv + 1) * 0.5, 2));
}

static int polkadot_offset_cmp(const void *p1, const void *p2) {
	const cmplx *o1 = p1;
	const cmplx *o2 = p2;
	real d1 = cabs(*o1);
	real d2 = cabs(*o2);

	if(d1 < d2) { return -1; }
	if(d2 < d1) { return  1; }

	// stabilize sort
	if(p1 < p2) { return -1; }
	return 1;
}

static int make_polkadot_circle(int subdiv, cmplx out_offsets[]) {
	int c = 0;
	cmplx origin = CMPLX(0.5, 0.5);

	for(int i = 0; i <= subdiv; ++i) {
		for(int j = 0; j <= subdiv; ++j) {
			cmplx ofs = ((j + 0.5 * (i & 1)) / subdiv) + I * (i / (real)subdiv) - origin;

			if(cabs(ofs) < 0.5) {
				out_offsets[c++] = ofs * 2;
			}
		}
	}

	qsort(out_offsets, c, sizeof(*out_offsets), polkadot_offset_cmp);
	return c;
}

static int build_path(int num_offsets, cmplx offsets[num_offsets], int path[num_offsets]) {
	for(int i = 0; i < num_offsets; ++i) {
		path[i] = i;
	}

	int first = rng_i32_range(0, num_offsets / 4);
	int tmp = path[first];
	path[first] = path[0];
	path[0] = tmp;

	int num_sorted = 1;
	int x = 0;

	// TODO compute these based on the subdivision factor
	real random_offset = 0.036;
	real max_dist = 0.2;

	while(num_sorted < num_offsets) {
		++x;
		cmplx ref = offsets[path[num_sorted - 1]] + rng_dir() * random_offset;
		real dist = 2;
		int nearest = num_offsets;

		for(int i = num_sorted; i < num_offsets; ++i) {
			real d = cabs(offsets[path[i]] - ref);
			if(d < dist) {
				dist = d;
				nearest = i;
			}
		}

		if(dist > max_dist) {
			return x;
		}

		assume(nearest < num_offsets);

		int tmp = path[num_sorted];
		path[num_sorted] = path[nearest];
		path[nearest] = tmp;

		++num_sorted;
	}

	return x;
}

typedef struct InfnetAnimData {
	BoxedProjectileArray *dots;
	cmplx *offsets;
	int *particle_spawn_times;
	cmplx origin;
	real radius;
	int num_dots;
} InfnetAnimData;

static void emit_particle(InfnetAnimData *infnet, int index, Projectile *dot, Sprite *spr) {
	if(dot->ent.draw_layer == LAYER_NODRAW) {
		return;
	}

	if(global.frames - infnet->particle_spawn_times[index] < 6) {
		return;
	}

	PARTICLE(
		.sprite_ptr = spr,
		.pos = dot->pos,
		.color = color_mul_scalar(COLOR_COPY(&dot->color), 0.75),
		.timeout = 10,
		.draw_rule = pdraw_timeout_scalefade(0.5, 1, 1, 0),
		.angle = rng_angle(),
	);

	infnet->particle_spawn_times[index] = global.frames;
}

TASK(infnet_lasers, { InfnetAnimData *infnet; }) {
	InfnetAnimData *infnet = ARGS.infnet;
	int path[infnet->num_dots];
	int path_size = build_path(infnet->num_dots, infnet->offsets, path);

	BoxedLaser lasers[path_size - 1];
	memset(&lasers, 0, sizeof(lasers));

	int lasers_spawned = 0;
	int lasers_alive = 0;

	Sprite *stardust_spr = res_sprite("part/stardust");

	while(!lasers_spawned || lasers_alive) {
		lasers_alive = 0;

		for(int i = 0; i < path_size - 1; ++i) {
			int index0 = path[i];
			int index1 = path[i + 1];

			Projectile *a = infnet->dots->size > index0 ? ENT_ARRAY_GET(infnet->dots, index0) : NULL;
			Projectile *b = infnet->dots->size > index1 ? ENT_ARRAY_GET(infnet->dots, index1) : NULL;

			if(a && b) {
				if(!lasers[i].ent) {
					int delay = 5 * i;
					Laser *l = create_laserline_ab(0, 0, 10, 60+delay, 180+delay, RGBA(1, 0.4, 0.1, 0));
					lasers[i] = ENT_BOX(l);
					++lasers_spawned;
				}

				Laser *l = ENT_UNBOX(lasers[i]);
				if(l) {
					l->pos = a->pos;
					l->args[0] = (b->pos - a->pos) * 0.005;
					emit_particle(infnet, index0, a, stardust_spr);
					emit_particle(infnet, index1, b, stardust_spr);

					// FIXME this optimization is not correct, because sometimes the laser may cross the viewport,
					// even though both endpoints are invisible. Maybe it's worth detecting this case.
					#if 0
					if(a->ent.draw_layer == LAYER_NODRAW && b->ent.draw_layer == LAYER_NODRAW) {
						l->ent.draw_layer = LAYER_NODRAW;
					} else {
						l->ent.draw_layer = LAYER_LASER_HIGH;
					}
					#endif
				}
			} else {
				Laser *l = lasers[i].ent ? ENT_UNBOX(lasers[i]) : NULL;
				if(l) {
					clear_laser(l, CLEAR_HAZARDS_FORCE);
				}
			}

			if(lasers[i].ent && ENT_UNBOX(lasers[i])) {
				++lasers_alive;
			}
		}

		YIELD;
	}

	log_debug("All lasers died!");
}

TASK(animate_infnet, { InfnetAnimData *infnet; }) {
	InfnetAnimData *infnet = ARGS.infnet;

	int i = 0;
	real t = 0;

	real mtime = 4000;
	real radius0 = hypot(VIEWPORT_W, VIEWPORT_H) * 2;
	real radius1 = 380;

	infnet->radius = radius0;

	do {
		YIELD;

		infnet->radius = lerp(radius0, radius1, fmin(1, t / mtime));

		ENT_ARRAY_FOREACH_COUNTER(infnet->dots, i, Projectile *dot, {
			dot->pos = infnet->radius * infnet->offsets[i] + infnet->origin;
			real rot_phase = (global.frames - dot->birthtime) / 300.0;
			// real color_phase = psin(t / 60.0 + M_PI * i / (infnet->num_dots - 1.0));
			dot->color = *color_lerp(RGBA(1, 0, 0, 0), RGBA(0, 1, 0, 0), pcos(rot_phase));

			cmplx pivot = 0.2 * cdir(t/30.0);
			infnet->offsets[i] -= pivot;
			infnet->offsets[i] *= cdir(M_PI/400 * sin(rot_phase));
			infnet->offsets[i] += pivot;

			if(projectile_in_viewport(dot)) {
				dot->flags &= ~PFLAG_NOCOLLISION;
				dot->ent.draw_layer = LAYER_BULLET;
			} else {
				dot->flags |= PFLAG_NOCOLLISION;
				dot->ent.draw_layer = LAYER_NODRAW;
			}
		});

		capproach_asymptotic_p(&infnet->origin, global.plr.pos, 0.001, 1e-9);
		t += 1;
	} while(i > 0);
}

TASK(infinity_net, {
	BoxedBoss boss;
	cmplx origin;
}) {
	TASK_BIND(ARGS.boss);

	int spawntime = 120;
	int subdiv = 22;

	cmplx offsets[polkadot_circle_approx_size_upper_bound(subdiv)];
	int num_offsets = make_polkadot_circle(subdiv, offsets);
	log_debug("%i / %zi", num_offsets, ARRAY_SIZE(offsets));
	assume(num_offsets <= ARRAY_SIZE(offsets));

	DECLARE_ENT_ARRAY(Projectile, dots, num_offsets);

	int particle_spawn_times[num_offsets];
	memset(particle_spawn_times, 0, sizeof(particle_spawn_times));

	InfnetAnimData infnet;
	infnet.offsets = offsets;
	infnet.particle_spawn_times = particle_spawn_times;
	infnet.num_dots = num_offsets;
	infnet.origin = ARGS.origin;
	infnet.dots = &dots;

	INVOKE_SUBTASK(animate_infnet, &infnet);

	for(int i = 0; i < num_offsets; ++i) {
		ENT_ARRAY_ADD(&dots, PROJECTILE(
			.proto = pp_ball,
			.color = color_lerp(RGBA(1, 0, 0, 0), RGBA(0, 0, 1, 0), i / (num_offsets - 1.0)),
			.pos = ARGS.origin + infnet.radius * offsets[i],
			.flags = PFLAG_INDESTRUCTIBLE | PFLAG_NOCLEAR | PFLAG_NOAUTOREMOVE,
			// .flags = PFLAG_NOCLEAR,
		));
		if(i % (num_offsets / spawntime) == 0) YIELD;
	}

	for(;;) {
		INVOKE_SUBTASK(infnet_lasers, &infnet);
		WAIT(170);
	}
}

DEFINE_EXTERN_TASK(stagex_spell_infinity_network) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);

	INVOKE_SUBTASK(infinity_net, ENT_BOX(boss), (VIEWPORT_W+VIEWPORT_H*I)*0.5);

	STALL;
}
