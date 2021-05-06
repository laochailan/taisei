/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "global.h"
#include "elly.h"
#include "draw.h"

Scythe scythe_create(cmplx pos) {
	return (Scythe){ .pos = pos, .scale = 1 };
}

static void scythe_particles(Scythe *s) {
	PARTICLE(
		.sprite_ptr = res_sprite("stage6/scythe"),
		.pos = s->pos+I*6*sin(global.frames/25.0),
		.draw_rule = pdraw_timeout_fade(1, 0),
		.timeout = 8,
		.angle = s->angle,
		.scale = s->scale,
		.flags = PFLAG_REQUIREDPARTICLE,
	);

	RNG_ARRAY(rand, 2);
	PARTICLE(
		.sprite = "smoothdot",
		.pos = s->pos + 100 * s->scale * vrng_real(rand[0]) * vrng_dir(rand[1]),
		.color = RGBA(1.0, 0.1, 1.0, 0.0),
		.draw_rule = pdraw_timeout_scalefade(0.4, 2, 1, 0),
		.timeout = 60,
		.flags = PFLAG_REQUIREDPARTICLE,
	);
}

DEFINE_EXTERN_TASK(scythe_update_loop) {
	for(;; YIELD) {
		scythe_particles(ARGS.scythe);
		move_update(&ARGS.scythe->pos, &ARGS.scythe->move);
		ARGS.scythe->angle += ARGS.scythe->angular_velocity;
	}
}

void scythe_draw(Enemy *e, int t, bool render) {
}

// REMOVE
void scythe_common(Enemy *e, int t) {
	e->args[1] += cimag(e->args[1]);
}

// REMOVE
int wait_proj(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	if(t > creal(p->args[1])) {
		if(t == creal(p->args[1]) + 1) {
			play_sfx_ex("redirect", 4, false);
			spawn_projectile_highlight_effect(p);
		}

		p->angle = carg(p->args[0]);
		p->pos += p->args[0];
	}

	return ACTION_NONE;
}


int scythe_infinity(Enemy *e, int t) {
	if(t < 0) {
		scythe_common(e, t);
		return 1;
	}

	TIMER(&t);
	FROM_TO(0, 40, 1) {
		GO_TO(e, VIEWPORT_W/2+200.0*I, 0.01);
		e->args[2] = fmin(0.8, creal(e->args[2])+0.0003*t*t);
		e->args[1] = creal(e->args[1]) + I*fmin(0.2, cimag(e->args[1])+0.0001*t*t);
	}

	FROM_TO_SND("shot1_loop",40, 3000, 1) {
		float w = fmin(0.15, 0.0001*(t-40));
		e->pos = VIEWPORT_W/2 + 200.0*I + 200*cos(w*(t-40)+M_PI/2.0) + I*80*sin(creal(e->args[0])*w*(t-40));

		PROJECTILE(
			.proto = pp_ball,
			.pos = e->pos+80*cexp(I*creal(e->args[1])),
			.color = RGB(cos(creal(e->args[1])), sin(creal(e->args[1])), cos(creal(e->args[1])+2.1)),
			.rule = asymptotic,
			.args = {
				(1+0.4*global.diff)*cexp(I*creal(e->args[1])),
				3 + 0.2 * global.diff
			}
		);
	}

	scythe_common(e, t);
	return 1;
}

int scythe_reset(Enemy *e, int t) {
	if(t < 0) {
		scythe_common(e, t);
		return 1;
	}

	if(t == 1)
		e->args[1] = fmod(creal(e->args[1]), 2*M_PI) + I*cimag(e->args[1]);

	GO_TO(e, BOSS_DEFAULT_GO_POS, 0.05);
	e->args[2] = fmax(0.6, creal(e->args[2])-0.01*t);
	e->args[1] += (0.19-creal(e->args[1]))*0.05;
	e->args[1] = creal(e->args[1]) + I*0.9*cimag(e->args[1]);

	scythe_common(e, t);
	return 1;
}

attr_returns_nonnull
Enemy* find_scythe(void) {
	for(Enemy *e = global.enemies.first; e; e = e->next) {
		if(e->visual_rule == scythe_draw) {
			return e;
		}
	}

	UNREACHABLE;
}

void elly_clap(Boss *b, int claptime) {
	aniplayer_queue(&b->ani, "clapyohands", 1);
	aniplayer_queue_frames(&b->ani, "holdyohands", claptime);
	aniplayer_queue(&b->ani, "unclapyohands", 1);
	aniplayer_queue(&b->ani, "main", 0);
}

static int baryon_unfold(Enemy *baryon, int t) {
	if(t < 0)
		return 1; // not catching death for references! because there will be no death!

	TIMER(&t);

	int extent = 100;
	float timeout;

	if(global.stage->type == STAGE_SPELL) {
		timeout = 92;
	} else {
		timeout = 142;
	}

	FROM_TO(0, timeout, 1) {
		for(Enemy *e = global.enemies.first; e; e = e->next) {
			if(e->visual_rule == baryon_center_draw) {
				float f = t / (float)timeout;
				float x = f;
				float g = sin(2 * M_PI * log(log(x + 1) + 1));
				float a = g * pow(1 - x, 2 * x);
				f = 1 - pow(1 - f, 3) + a;

				baryon->pos = baryon->pos0 = e->pos + baryon->args[0] * f * extent;
				return 1;
			}
		}
	}

	return 1;
}

static int elly_baryon_center(Enemy *e, int t) {
	if(t == EVENT_DEATH) {
		free_ref(creal(e->args[1]));
		free_ref(cimag(e->args[1]));
	}

	if(global.boss) {
		e->pos = global.boss->pos;
	}

	return 1;
}

int scythe_explode(Enemy *e, int t) {
	if(t < 0) {
		scythe_common(e, t);
		return 0;
	}

	if(t < 50) {
		e->args[1] += 0.001*I*t;
		e->args[2] += 0.0015*t;
	}

	if(t >= 50)
		e->args[2] -= 0.002*(t-50);

	if(t == 100) {
		petal_explosion(100, e->pos);
		stage_shake_view(16);
		play_sfx("boom");

		scythe_common(e, t);
		return ACTION_DESTROY;
	}

	scythe_common(e, t);
	return 1;
}

void elly_spawn_baryons(cmplx pos) {
	int i;
	Enemy *e, *last = NULL, *first = NULL, *middle = NULL;

	for(i = 0; i < 6; i++) {
		e = create_enemy3c(pos, ENEMY_IMMUNE, baryon, baryon_unfold, 1.5*cexp(2.0*I*M_PI/6*i), i != 0 ? add_ref(last) : 0, i);
		e->ent.draw_layer = LAYER_BACKGROUND;

		if(i == 0) {
			first = e;
		} else if(i == 3) {
			middle = e;
		}

		last = e;
	}

	first->args[1] = add_ref(last);
	e = create_enemy2c(pos, ENEMY_IMMUNE, baryon_center_draw, elly_baryon_center, 0, add_ref(first) + I*add_ref(middle));
	e->ent.draw_layer = LAYER_BACKGROUND;
}


void set_baryon_rule(EnemyLogicRule r) {
	Enemy *e;

	for(e = global.enemies.first; e; e = e->next) {
		if(e->visual_rule == baryon) {
			e->birthtime = global.frames;
			e->logic_rule = r;
		}
	}
}

int baryon_reset(Enemy* baryon, int t) {
	if(t < 0) {
		return 1;
	}

	for(Enemy *e = global.enemies.first; e; e = e->next) {
		if(e->visual_rule == baryon_center_draw) {
			cmplx targ_pos = baryon->pos0 - e->pos0 + e->pos;
			GO_TO(baryon, targ_pos, 0.1);

			return 1;
		}
	}

	GO_TO(baryon, baryon->pos0, 0.1);
	return 1;
}

