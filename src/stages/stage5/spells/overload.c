/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"

static Enemy* overload_find_next_node(cmplx from, double playerbias) {
	Enemy *nearest = NULL, *e;
	double dist, mindist = INFINITY;

	cmplx org = from + playerbias * cnormalize(global.plr.pos - from);

	for(e = global.enemies.first; e; e = e->next) {
		if(e->args[2]) {
			continue;
		}

		dist = cabs(e->pos - org);

		if(dist < mindist) {
			nearest = e;
			mindist = dist;
		}
	}

	return nearest;
}

static void overload_node_visual(Enemy *e, int t, bool render) {
	if(render) {
		return;
	}

	// TODO: removing e->args[2] here lets you see where the "nodes" are traveling to
	//if(e->args[2] && !(t % 5)) {
	if(!(t % 5)) {
		RNG_ARRAY(rand, 2);
		cmplx offset = vrng_sreal(rand[0]) * 15.0 + vrng_sreal(rand[1]) * 10.0 * I;
		PARTICLE(
			.sprite_ptr = res_sprite("part/smoothdot"),
			.pos = e->pos + offset,
			// orange and teal dots
			.color = e->args[1] ? RGBA(1.0, 0.5, 0.0, 0.0) : RGBA(0.0, 0.5, 0.5, 0.0),
			.timeout = 50,
			.draw_rule = pdraw_timeout_scalefade(1.2, 0, 0.4, 0),
			.move = move_towards(e->pos - 70 * I, 0.03),
		);
	}
}

TASK(overload_trigger_bullet, { Enemy *e; ProjPrototype *proto; cmplx pos; cmplx vel; }) {
	Enemy *target = ARGS.e;
	Projectile *p = TASK_BIND(PROJECTILE(
		.proto = ARGS.proto,
		.pos = ARGS.pos,
		.color = RGBA(0.2, 0.2, 1.0, 0.0),
		.flags = PFLAG_NOCLEAR,
	));
	p->move = move_linear(2 * cnormalize(target->pos - ARGS.pos));

	for(int t = 0;;t++, YIELD) {
		if(cabs(p->pos - target->pos) < 5) {
			p->move.velocity = 0;
			p->pos = target->pos;
			target->args[1] = 1;
			p->args[2] = difficulty_value(50, 45, 40, 35);
			target->args[3] = global.frames + p->args[2];

			play_sfx("shot_special1");

			int count = difficulty_value(8, 10, 12, 14);
			for(int i = 0; i < count; ++i) {
				cmplx dir = cdir(t + i * M_TAU/count);
				PROJECTILE(
					.proto = pp_bigball,
					.pos = p->pos,
					.color = RGBA(1.0, 0.5, 0.0, 0.0),
					.move = move_asymptotic_simple(1.1 * dir, 5),
				);
				PROJECTILE(
					.proto = pp_bigball,
					.pos = p->pos,
					.color = RGBA(0.0, 0.5, 1.0, 0.0),
					.move = move_asymptotic_simple(dir, 10),
				);
			}

			stage_shake_view(40);
			aniplayer_hard_switch(&global.boss->ani, "main_mirror", 0);
			play_sfx("boom");

			p->type = PROJ_DEAD;
			break;
		}

		p->angle = global.frames + t;
		RNG_ARRAY(rand, 4);
		PARTICLE(
			.sprite = rng_chance(0.5) ? "lightning0" : "lightning1",
			.pos = p->pos + 3 * (vrng_sreal(rand[0]) + I * vrng_sreal(rand[1])),
			.angle = vrng_sreal(rand[2]) * M_TAU,
			.color = RGBA(1.0, 0.7 + 0.2 * vrng_sreal(rand[3]), 0.4, 0.0),
			.timeout = 20,
			.draw_rule = pdraw_timeout_scalefade(0, 2.4, 2, 0),
		);
		play_sfx_loop("charge_generic");
	}
}

TASK(overload_fire_trigger_bullet, { BoxedBoss boss; }) {
	Boss *boss = TASK_BIND(ARGS.boss);
	Enemy *e = overload_find_next_node(boss->pos, 250);

	if(!e) {
		return;
	}

	aniplayer_hard_switch(&boss->ani, "dashdown_left", 1);
	aniplayer_queue(&boss->ani, "main", 0);

	INVOKE_TASK(overload_trigger_bullet, {
		.e = e,
		.pos = boss->pos,
		.proto = pp_soul,
		.vel = 3 * cnormalize(e->pos - boss->pos),
	});

	play_sfx("shot_special1");
	play_sfx("enemydeath");
	play_sfx("shot2");
}

TASK(overload, { BoxedBoss boss; BoxedEnemy e; cmplx epos; }) {
	Enemy *e = TASK_BIND(ARGS.e);

	for(int t = 0;;t++, YIELD) {
		if(e->args[1]) {
			if(creal(e->args[1]) < 2) {
				e->args[1] += 1;
				continue;
			}

			if(global.frames == creal(e->args[3])) {
				cmplx o2 = e->args[2];
				e->args[2] = 0;
				Enemy *new = overload_find_next_node(ARGS.epos, 75);
				e->args[2] = o2;

				if(new && e != new) {
					e->args[1] = 0;
					e->args[2] = new->args[2] = 600;
					new->args[1] = 1;
					new->args[3] = global.frames + difficulty_value(55, 50, 45, 40);

					// TODO: the lasers seemingly "work" insofar as the nodes turn orange when you'd expect
					// but there's no laser effect
					Laser *l = create_laserline_ab(e->pos, new->pos, 10, 30, e->args[2], RGBA(0.3, 1, 1, 0));
					l->ent.draw_layer = LAYER_LASER_LOW;
					l->unclearable = true;

					if(global.diff > D_Easy) {
						int count = floor(global.diff * 2.5), i;
						real r = rng_real() * M_TAU;

						for(i = 0; i < count; ++i) {
							PROJECTILE(
								.proto = pp_rice,
								.pos = e->args[0],
								.color = RGBA(1, 1, 0, 0),
								.move = move_asymptotic_simple(2 * cdir(r + i * M_TAU / count), 2),
							);
						}

						play_sfx("shot2");
					}

					play_sfx("redirect");
				} else {
					Enemy *o;
					Laser *l;
					int count = 6 + 2 * global.diff, i;

					e->args[2] = 1;

					for(o = global.enemies.first; o; o = o->next) {
						if(!o->args[2]) {
							continue;
						}

						for(i = 0; i < count; ++i) {
							PROJECTILE(
								.proto = pp_ball,
								.pos = o->args[0],
								.color = RGBA(0, 1, 1, 0),
								.move = move_asymptotic_simple(1.5 * cdir(t + i * M_TAU/count), 8),
							);
						}

						o->args[1] = 0;
						o->args[2] = 0;

						stage_shake_view(5);
					}

					for(l = global.lasers.first; l; l = l->next) {
						l->deathtime = global.frames - l->birthtime + 20;
					}
					play_sfx("boom");
					INVOKE_SUBTASK(overload_fire_trigger_bullet, ARGS.boss);
				}
			}
		}

		if(e->args[2]) {
			e->args[2] -= 1;
		}
	}
}

DEFINE_EXTERN_TASK(stage5_spell_overload) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_towards(VIEWPORT_W/2 + 50.0 * I, 0.02);
	BEGIN_BOSS_ATTACK(&ARGS);

	real count = 5;
	real step = VIEWPORT_W / count;
	for(int i = 0; i < count; i++) {
		for(int j = 0; j < count; j++) {
			cmplx epos = step * (0.5 + i) + (step * j + 125) * I;
			Enemy *e = create_enemy_p(&global.enemies, boss->pos, ENEMY_IMMUNE, overload_node_visual, NULL, 0, 0, 0, 1);
			e->move = move_towards(epos, 0.05);
			INVOKE_TASK(overload, ENT_BOX(boss), ENT_BOX(e), epos);
		}
	}
	WAIT(10);
	INVOKE_TASK(overload_fire_trigger_bullet, ARGS.boss);
}

