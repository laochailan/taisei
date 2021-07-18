/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "iku.h"

#include "stage5.h"

#include "common_tasks.h"

static void iku_slave_draw(EntityInterface *e) {
	IkuSlave *slave = ENT_CAST(e, IkuSlave);
	r_draw_sprite(&(SpriteParams) {
		.pos.as_cmplx = slave->pos,
		.sprite_ptr = slave->sprites.cloud,
		.scale.as_cmplx = slave->scale,
		.color = &slave->color,
	});
}

void iku_lightning_particle(cmplx pos) {
	char *part = rng_chance(0.5) ? "lightning0" : "lightning1";
	PARTICLE(
		.sprite = part,
		.pos = pos,
		.color = RGBA(1.0, 1.0, 1.0, 0.0),
		.timeout = 20,
		.draw_rule = pdraw_timeout_fade(0.8, 0.0),
		.angle = rng_real() * M_TAU,
	);
}

TASK(iku_slave_visual, { BoxedIkuSlave slave; }) {
	IkuSlave *slave = TASK_BIND(ARGS.slave);

	int period = 3;
	WAIT(rng_irange(0, period));

	for(;;WAIT(period)) {
		RNG_ARRAY(rand, 2);
		cmplx offset = vrng_sreal(rand[0]) * 5 + vrng_sreal(rand[1]) * 5 * I;
		iku_lightning_particle(slave->pos);
		PARTICLE(
			.sprite_ptr = slave->sprites.cloud,
			.pos = slave->pos,
			.color = &slave->color,
			.draw_rule = pdraw_timeout_fade(2, 0),
			.timeout = 50,
			.move = move_linear(offset * 0.2),
			.flags = PFLAG_REQUIREDPARTICLE,
		);
	}
}

void stage5_init_iku_slave(IkuSlave *slave, cmplx pos) {
	slave->pos = pos;
	slave->spawn_time = global.frames;
	slave->ent.draw_layer = LAYER_BOSS - 1;
	slave->ent.draw_func = iku_slave_draw;
	slave->scale = (1 + I);
	slave->color = *RGBA_MUL_ALPHA(0.1, 0.1, 0.6, 0.2);
	slave->sprites.lightning0 = res_sprite("part/lightning0");
	slave->sprites.lightning1 = res_sprite("part/lightning1");
	slave->sprites.cloud = res_sprite("part/lightningball");
	slave->sprites.dot = res_sprite("part/smoothdot");

	INVOKE_TASK(iku_slave_visual, ENT_BOX(slave));
}

IkuSlave *stage5_midboss_slave(cmplx pos) {
	IkuSlave *slave = TASK_HOST_ENT(IkuSlave);
	TASK_HOST_EVENTS(slave->events);
	stage5_init_iku_slave(slave, pos);

	return slave;
}

DEFINE_EXTERN_TASK(iku_slave_move) {
	IkuSlave *slave = TASK_BIND(ARGS.slave);

	for(;; YIELD) {
		move_update(&slave->pos, &ARGS.move);
	}
}

static void iku_nonspell_spawn_cloud(void) {
	RNG_ARRAY(rand, 4);
	float v = (vrng_sreal(rand[2]) + vrng_sreal(rand[3])) * 0.5 + 1.0;

	PROJECTILE(
		// FIXME: add prototype, or shove it into the basic ones somehow,
		// or just replace this with some thing else
		.sprite_ptr = res_sprite("part/lightningball"),
		.size = 48 * (1 + I),
		.collision_size = 21.6 * (1 + I),
		.pos = VIEWPORT_W * vrng_sreal(rand[0]) - 15.0 * I,
		.color = RGBA_MUL_ALPHA(0.2, 0.0, 0.4, 0.6),
		.move = move_accelerated(1 - 2 * vrng_sreal(rand[1]) + v * I, -0.01 * I),
		.shader = "sprite_default",
	);
}

Boss *stage5_spawn_iku(cmplx pos) {
	Boss *iku = create_boss("Nagae Iku", "iku", pos);
	boss_set_portrait(iku, "iku", NULL, "normal");
	iku->glowcolor = *RGB(0.2, 0.4, 0.5);
	iku->shadowcolor = *RGBA_MUL_ALPHA(0.65, 0.2, 0.75, 0.5);
	return iku;
}

DEFINE_EXTERN_TASK(iku_induction_bullet) {
	Projectile *p = TASK_BIND(ARGS.p);
	cmplx radial_vel = ARGS.radial_vel;
	cmplx angular_vel = ARGS.angular_vel;

	for(int time = 0;; time++, YIELD) {
		int t = time * sqrt(global.diff);
		if(global.diff > D_Normal && ARGS.mode) {
			t = time*0.6;
			t = 230-t;
		}

		p->pos = p->pos0 + radial_vel * t * cexp(angular_vel * t);

		if(time == 0) {
			// don't lerp; the spawn position is very different on hard/lunatic and would cause false hits
			p->prevpos = p->pos;
		}

		// calculate angle manually using the derivative of p->pos
		p->angle = carg(radial_vel * cexp(angular_vel * t) * (1 + angular_vel * t));
	}
}

DEFINE_EXTERN_TASK(iku_spawn_clouds) {
	for(int i = 0;; i += WAIT(2)) {
		iku_nonspell_spawn_cloud();
	}
}
