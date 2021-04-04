/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"

MODERNIZE_THIS_FILE_AND_REMOVE_ME

static int zigzag_bullet(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	int l = 50;
	p->pos = p->pos0+(abs(((2*t)%l)-l/2)*I+t)*2*p->args[0];

	if(t%2 == 0) {
		PARTICLE(
			.sprite = "lightningball",
			.pos = p->pos,
			.color = RGBA(0.1, 0.1, 0.6, 0.0),
			.timeout = 15,
			.draw_rule = Fade,
		);
	}

	return ACTION_NONE;
}

static int lightning_slave(Enemy *e, int t) {
	if(t < 0)
		return 1;
	if(t > 200)
		return ACTION_DESTROY;

	TIMER(&t);

	e->pos += e->args[0];

	FROM_TO(0,200,20)
		e->args[0] *= cexp(I * (0.25 + 0.25 * frand() * M_PI));

	FROM_TO(0, 200, 3)
		if(cabs(e->pos-global.plr.pos) > 60) {
			Color *clr = RGBA(1-1/(1+0.01*_i), 0.5-0.01*_i, 1, 0);

			Projectile *p = PROJECTILE(
				.proto = pp_wave,
				.pos = e->pos,
				.color = clr,
				.rule = asymptotic,
				.args = {
					0.75*e->args[0]/cabs(e->args[0])*I,
					10
				},
			);

			if(projectile_in_viewport(p)) {
				for(int i = 0; i < 3; ++i) {
					tsrand_fill(2);
					iku_lightning_particle(p->pos + 5 * afrand(0) * cexp(I*M_PI*2*afrand(1)));
				}

				play_sfx_ex("shot3", 0, false);
			}
		}

	return 1;
}

void iku_lightning(Boss *b, int time) {
	int t = time % 141;

	if(time == EVENT_DEATH) {
		enemy_kill_all(&global.enemies);
		return;
	}

	if(time < 0) {
		GO_TO(b, BOSS_DEFAULT_GO_POS, 0.03);
		return;
	}

	TIMER(&t);

	GO_TO(b,VIEWPORT_W/2+tanh(sin(time/100))*200+I*VIEWPORT_H/3+I*(cos(t/200)-1)*50,0.03);

	AT(0) {
		play_sound("charge_generic");
	}

	FROM_TO(0, 60, 1) {
		cmplx n = cexp(2.0*I*M_PI*frand());
		float l = 150*frand()+50;
		float s = 4+_i*0.01;
		float alpha = 0.5;

		PARTICLE(
			.sprite = "lightningball",
			.pos = b->pos+l*n,
			.color = RGBA(0.1*alpha, 0.1*alpha, 0.6*alpha, 0),
			.draw_rule = Fade,
			.rule = linear,
			.timeout = l/s,
			.args = { -s*n },
		);
	}

	if(global.diff >= D_Hard && time > 0 && !(time%100)) {
		int c = 7 + 2 * (global.diff == D_Lunatic);
		for(int i = 0; i<c; i++) {
			PROJECTILE(
				.proto = pp_bigball,
				.pos = b->pos,
				.color = RGBA(0.5, 0.1, 1.0, 0.0),
				.rule = zigzag_bullet,
				.args = { cexp(2*M_PI*I/c*i+I*carg(global.plr.pos-b->pos)) },
			);
		}

		play_sound("redirect");
		play_sound("shot_special1");
	}

	AT(100) {
		aniplayer_hard_switch(&b->ani, ((time/141)&1) ? "dashdown_left" : "dashdown_right",1);
		aniplayer_queue(&b->ani, "main", 0);
		int c = 40;
		int l = 200;
		int s = 10;

		for(int i=0; i < c; i++) {
			cmplx n = cexp(2.0*I*M_PI*frand());
			PARTICLE(
				.sprite = "smoke",
				.pos = b->pos,
				.color = RGBA(0.4, 0.4, 1.0, 0.0),
				.draw_rule = Fade,
				.rule = linear,
				.timeout = l/s,
				.args = { s*n },
			);
		}

		for(int i = 0; i < global.diff+1; i++){
			create_enemy1c(b->pos, ENEMY_IMMUNE, NULL, lightning_slave, 10*cexp(I*carg(global.plr.pos - b->pos)+2.0*I*M_PI/(global.diff+1)*i));
		}

		play_sound("shot_special1");
	}
}
