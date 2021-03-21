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


void stage5_init_iku_slave(IkuSlave *slave, cmplx pos) {
	slave->pos = pos;
	slave->spawn_time = global.frames;

}

IkuSlave *stage5_midboss_slave(cmplx pos) {
	IkuSlave *slave = TASK_HOST_ENT(IkuSlave);
	TASK_HOST_EVENTS(slave->events);
	stage5_init_iku_slave(slave, pos);

	float alpha = 1;
	Color *clr = RGBA_MUL_ALPHA(0.1*alpha, 0.1*alpha, 0.6*alpha, 0.5*alpha);
	clr->a = 0;
	slave->color = *clr;
	slave->scale = (1 + I) * 0.7;
	// TODO: you were here
	// need to continue porting vvvv that stuff into this functioning
	// including the PARTICLE

	return slave;
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

void iku_slave_visual(Enemy *e, int t, bool render) {
	if(render) {
		return;
	}

	cmplx offset = (frand()-0.5)*10 + (frand()-0.5)*10.0*I;

	if(e->args[2] && !(t % 5)) {
		iku_lightning_particle(e->pos + 3*offset, t);
	}

	if(!(t % 3)) {
		float alpha = 1;

		if(!e->args[2]) {
			alpha *= 0.03;
		}

		Color *clr = RGBA_MUL_ALPHA(0.1*alpha, 0.1*alpha, 0.6*alpha, 0.5*alpha);
		clr->a = 0;

		PARTICLE(
			.sprite = "lightningball",
			.pos = 0,
			.color = clr,
			.draw_rule = Fade,
			.rule = enemy_flare,
			.timeout = 50,
			.args = { offset*0.1, add_ref(e) },
			.flags = PFLAG_REQUIREDPARTICLE,
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
