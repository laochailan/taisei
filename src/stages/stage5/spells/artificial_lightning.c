/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"

TASK(lightningball_particle, { BoxedBoss boss; }) {
	Boss *boss = TASK_BIND(ARGS.boss);
	for(int x = 0;; x++, YIELD) {
		cmplx n = cdir(M_TAU * rng_real());
		float l = 150 * rng_real() + 50;
		float s = 4 + x * 0.01;

		PARTICLE(
			.sprite = "lightningball",
			.pos = boss->pos,
			.color = RGBA(0.05, 0.05, 0.3, 0),
			.draw_rule = pdraw_timeout_fade(0.5, 0),
			.timeout = l/s,
			.move = move_linear(-s * n),
		);
	}
}

TASK(zigzag_move, { BoxedProjectile p; cmplx velocity; }) {
	Projectile *p = TASK_BIND(ARGS.p);

	for(int time = 0;; time++, YIELD) {
		int zigtime = 50;
		p->pos = p->pos0 + (abs(((2 * time) % zigtime) - zigtime / 2) * I + time) * ARGS.velocity;

		if(time % 2 == 0) {
			PARTICLE(
				.sprite = "lightningball",
				.pos = p->pos,
				.color = RGBA(0.1, 0.1, 0.6, 0.0),
				.timeout = 15,
				.draw_rule = pdraw_timeout_fade(0.5, 0),
			);
		}
	}
}

TASK(zigzag_shoot, { BoxedBoss boss; } ) {
	Boss *boss = TASK_BIND(ARGS.boss);
	for(int time = 0;;) {
		time += WAIT(100);
		int count = difficulty_value(7, 7, 7, 9);
		for(int i = 0; i < count; i++) {
			Projectile *p = PROJECTILE(
				.proto = pp_bigball,
				.pos = boss->pos,
				.color = RGBA(0.5, 0.1, 1.0, 0.0),
			);
			INVOKE_TASK(zigzag_move, ENT_BOX(p), cdir(M_TAU / count * i) * cnormalize(global.plr.pos - boss->pos));
		}
		play_sfx("redirect");
		play_sfx("shot_special1");
	}
}

TASK(lightning_slave_move, { BoxedEnemy e; cmplx velocity; }) {
	Enemy *e = TASK_BIND(ARGS.e);
	cmplx velocity = ARGS.velocity;

	for(int time = 0;; time++, YIELD) {
		e->move = move_linear(velocity);
		if(time%20 == 0) {
			velocity *= cdir(0.25 + 0.25 * rng_real() * M_PI);
		}
	}
}

TASK(lightning_slave, { cmplx pos; cmplx move_arg; }) {
	Enemy *e = TASK_BIND(create_enemy1c(ARGS.pos, 1, NULL, NULL, 0));
	e->flags = EFLAGS_GHOST;

	INVOKE_TASK(lightning_slave_move, ENT_BOX(e), ARGS.move_arg);

	for(int x = 0; x < 67; x++, WAIT(3)) {
		if(cabs(e->pos - global.plr.pos) > 60) {
			Color *clr = RGBA(1 - 1 / (1 + 0.01 * x), 0.5 - 0.01 * x, 1, 0);

			Projectile *p = PROJECTILE(
				.proto = pp_wave,
				.pos = e->pos,
				.color = clr,
				.move = move_asymptotic_simple(0.75 * e->move.velocity / cabs(e->move.velocity) * I, 10),
			);

			if(projectile_in_viewport(p)) {
				for(int i = 0; i < 3; ++i) {
					RNG_ARRAY(rand, 2);
					iku_lightning_particle(p->pos + 5 * vrng_sreal(rand[0]) * vrng_dir(rand[1]));
				}
				play_sfx_ex("shot3", 0, false);
			}
		}
	}
}

TASK(lightning_slaves, { BoxedBoss boss; } ) {
	Boss *boss = TASK_BIND(ARGS.boss);
	for(int time = 0;; time++, YIELD) {
		aniplayer_hard_switch(&boss->ani, ((time/141)&1) ? "dashdown_left" : "dashdown_right",1);
		aniplayer_queue(&boss->ani, "main", 0);

		int c = 40;
		int l = 200;
		int s = 10;
		if(time % 100 == 0) {
			for(int i = 0; i < c; i++) {
				cmplx n = cdir(M_TAU * rng_real());
				PARTICLE(
					.sprite = "smoke",
					.pos = boss->pos,
					.color = RGBA(0.4, 0.4, 1.0, 0.0),
					.move = move_linear(s * n),
					.draw_rule = pdraw_timeout_fade(0.5, 0),
					.timeout = l/s,
				);
			}

			int count = difficulty_value(2, 3, 4, 5);
			for(int i = 0; i < count; i++){
				INVOKE_SUBTASK(lightning_slave, boss->pos, 10 * cnormalize(global.plr.pos - boss->pos) * cdir(M_TAU / count * i));
			}

			play_sfx("shot_special1");
		}
	}
}

DEFINE_EXTERN_TASK(stage5_spell_artificial_lightning) {
	STAGE_BOOKMARK(artificial-lightning);
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_towards(VIEWPORT_W/2 + 200.0 * I, 0.06);
	BEGIN_BOSS_ATTACK(&ARGS);

	Rect wander_bounds = viewport_bounds(64);
	wander_bounds.top += 64;
	wander_bounds.bottom = VIEWPORT_H * 0.4;

	play_sfx("charge_generic");

	INVOKE_SUBTASK(lightningball_particle, ENT_BOX(boss));
	if(global.diff >= D_Hard) {
		INVOKE_SUBTASK(zigzag_shoot, ENT_BOX(boss));
	}
	INVOKE_SUBTASK(lightning_slaves, ENT_BOX(boss));

	boss->move.attraction = 0.03;
	for(;;WAIT(100)) {
		boss->move.attraction_point = common_wander(boss->pos, VIEWPORT_W * 0.3, wander_bounds);
	}
}
