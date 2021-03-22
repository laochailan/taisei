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

MODERNIZE_THIS_FILE_AND_REMOVE_ME

static void iku_slave_draw(EntityInterface *e) {
	IkuSlave *slave = ENT_CAST(e, IkuSlave);
	//int time = global.frames - slave->spawn_time;
	r_draw_sprite(&(SpriteParams) {
		.pos.as_cmplx = slave->pos,
		.sprite_ptr = slave->sprites.lightning0,
		.scale.as_cmplx = slave->scale,
		.color = &slave->color,
	});
}

TASK(iku_slave_particles, { BoxedIkuSlave slave; }) {
	IkuSlave *slave = TASK_BIND(ARGS.slave);

	int period = 5;
	WAIT(rng_irange(0, period));

	for(;;WAIT(period)) {
		float alpha = 1;

		Color *clr = RGBA_MUL_ALPHA(0.1*alpha, 0.1*alpha, 0.6*alpha, 0.5*alpha);
		clr->a = 0;

		PARTICLE(
			.sprite_ptr = slave->sprites.particle,
			.pos = 0,
			.color = clr,
			.draw_rule = pdraw_timeout_scale(2, 0.01),
			.flags = PFLAG_REQUIREDPARTICLE,
		);
	}

}

void stage5_init_iku_slave(IkuSlave *slave, cmplx pos) {
	float alpha = 1;
	slave->pos = pos;
	slave->spawn_time = global.frames;
	slave->ent.draw_layer = LAYER_BOSS - 1;
	slave->ent.draw_func = iku_slave_draw;
	slave->scale = (1 + I) * 0.7;
	slave->color = *RGBA_MUL_ALPHA(0.1*alpha, 0.1*alpha, 0.6*alpha, 0.5*alpha);
	slave->sprites.lightning0 = res_sprite("part/lightning0");
	slave->sprites.lightning1 = res_sprite("part/lightning1");
	slave->sprites.particle = res_sprite("part/lightningball");

	INVOKE_TASK(iku_slave_particles, ENT_BOX(slave));
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

void iku_lightning_particle(cmplx pos, int t) {
	if(!(t % 5)) {
		char *part = frand() > 0.5 ? "lightning0" : "lightning1";
		PARTICLE(
			.sprite = part,
			.pos = pos,
			.color = RGBA(1.0, 1.0, 1.0, 0.0),
			.timeout = 20,
			.draw_rule = Fade,
			.flags = PFLAG_REQUIREDPARTICLE,
			.angle = frand()*2*M_PI,
		);
	}
}

void iku_nonspell_spawn_cloud(void) {
	tsrand_fill(4);
	float v = (afrand(2)+afrand(3))*0.5+1.0;

	PROJECTILE(
		// FIXME: add prototype, or shove it into the basic ones somehow,
		// or just replace this with some thing else
		.sprite_ptr = get_sprite("part/lightningball"),
		.size = 48 * (1+I),
		.collision_size = 21.6 * (1+I),

		.pos = VIEWPORT_W*afrand(0)-15.0*I,
		.color = RGBA_MUL_ALPHA(0.2, 0.0, 0.4, 0.6),
		.rule = accelerated,
		.args = {
			1-2*afrand(1)+v*I,
			-0.01*I
		},
		.shader = "sprite_default",
	);
}

static cmplx induction_bullet_traj(Projectile *p, float t) {
	return p->pos0 + p->args[0]*t*cexp(p->args[1]*t);
}

int iku_induction_bullet(Projectile *p, int time) {
	if(time < 0) {
		return ACTION_ACK;
	}

	float t = time*sqrt(global.diff);

	if(global.diff > D_Normal && !p->args[2]) {
		t = time*0.6;
		t = 230-t;
		if(t < 0)
			return ACTION_DESTROY;
	}

	p->pos = induction_bullet_traj(p,t);

	if(time == 0) {
		// don't lerp; the spawn position is very different on hard/lunatic and would cause false hits
		p->prevpos = p->pos;
	}

	p->angle = carg(p->args[0]*cexp(p->args[1]*t)*(1+p->args[1]*t));
	return 1;
}

Boss *stage5_spawn_iku(cmplx pos) {
	Boss *iku = create_boss("Iku Nagae", "iku", pos);
	boss_set_portrait(iku, "iku", NULL, "normal");
	iku->glowcolor = *RGB(0.2, 0.4, 0.5);
    iku->shadowcolor = *RGBA_MUL_ALPHA(0.65, 0.2, 0.75, 0.5);
	return iku;
}
