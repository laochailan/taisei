/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"

// TODO: this entire file
// ignore everything here for now

/* static Enemy* iku_overload_find_next_slave(cmplx from, double playerbias) { */
/* 	Enemy *nearest = NULL, *e; */
/* 	double dist, mindist = INFINITY; */

/* 	cmplx org = from + playerbias * cexp(I*(carg(global.plr.pos - from))); */

/* 	for(e = global.enemies.first; e; e = e->next) { */
/* 		if(e->args[2]) { */
/* 			continue; */
/* 		} */

/* 		dist = cabs(e->pos - org); */

/* 		if(dist < mindist) { */
/* 			nearest = e; */
/* 			mindist = dist; */
/* 		} */
/* 	} */

/* 	return nearest; */
/* } */

/* static int iku_overload_trigger_bullet(Projectile *p, int t) { */
/* 	if(t == EVENT_DEATH) { */
/* 		free_ref(p->args[1]); */
/* 		return ACTION_ACK; */
/* 	} */

/* 	if(t < 0) { */
/* 		return ACTION_ACK; */
/* 	} */

/* 	Enemy *target = REF(p->args[1]); */

/* 	if(!target) { */
/* 		return ACTION_DESTROY; */
/* 	} */

/* 	if(creal(p->args[2]) < 0) { */
/* 		linear(p, t); */
/* 		if(cabs(p->pos - target->pos) < 5) { */
/* 			p->pos = target->pos; */
/* 			target->args[1] = 1; */
/* 			p->args[2] = 55 - 5 * global.diff; */
/* 			target->args[3] = global.frames + p->args[2]; */
/* 			play_sound("shot_special1"); */
/* 		} */
/* 	} else { */
/* 		p->args[2] = approach(creal(p->args[2]), 0, 1); */
/* 		play_sfx_loop("charge_generic"); */
/* 	} */

/* 	if(creal(p->args[2]) == 0) { */
/* 		int cnt = 6 + 2 * global.diff; */
/* 		for(int i = 0; i < cnt; ++i) { */
/* 			cmplx dir = cexp(I*(t + i*2*M_PI/cnt)); */

/* 			PROJECTILE( */
/* 				.proto = pp_bigball, */
/* 				.pos = p->pos, */
/* 				.color = RGBA(1.0, 0.5, 0.0, 0.0), */
/* 				.rule = asymptotic, */
/* 				.args = { 1.1*dir, 5 }, */
/* 			); */

/* 			PROJECTILE( */
/* 				.proto = pp_bigball, */
/* 				.pos = p->pos, */
/* 				.color = RGBA(0.0, 0.5, 1.0, 0.0), */
/* 				.rule = asymptotic, */
/* 				.args = { dir, 10 }, */
/* 			); */
/* 		} */
/* 		stage_shake_view(40); */
/* 		aniplayer_hard_switch(&global.boss->ani,"main_mirror",0); */
/* 		play_sound("boom"); */
/* 		return ACTION_DESTROY; */
/* 	} */

/* 	p->angle = global.frames + t; */

/* 	tsrand_fill(5); */

/* 	PARTICLE( */
/* 		.sprite = afrand(0) > 0.5 ? "lightning0" : "lightning1", */
/* 		.pos = p->pos + 3 * (anfrand(1)+I*anfrand(2)), */
/* 		.angle = afrand(3) * 2 * M_PI, */
/* 		.color = RGBA(1.0, 0.7 + 0.2 * anfrand(4), 0.4, 0.0), */
/* 		.timeout = 20, */
/* 		.draw_rule = GrowFade, */
/* 		.args = { 0, 2.4 }, */
/* 	); */

/* 	return ACTION_NONE; */
/* } */

/* static void iku_overload_fire_trigger_bullet(void) { */
/* 	Enemy *e = iku_overload_find_next_slave(global.boss->pos, 250); */

/* 	aniplayer_hard_switch(&global.boss->ani,"dashdown_left",1); */
/* 	aniplayer_queue(&global.boss->ani,"main",0); */
/* 	if(!e) { */
/* 		return; */
/* 	} */

/* 	Boss *b = global.boss; */

/* 	PROJECTILE( */
/* 		.proto = pp_soul, */
/* 		.pos = b->pos, */
/* 		.color = RGBA(0.2, 0.2, 1.0, 0.0), */
/* 		.rule = iku_overload_trigger_bullet, */
/* 		.args = { */
/* 			3*cexp(I*carg(e->pos - b->pos)), */
/* 			add_ref(e), */
/* 			-1 */
/* 		}, */
/* 		.flags = PFLAG_NOCLEAR, */
/* 	); */

/* 	play_sound("shot_special1"); */
/* 	play_sound("enemydeath"); */
/* 	play_sound("shot2"); */
/* } */

/* static void iku_overload_slave_visual(Enemy *e, int t, bool render) { */
/* 	iku_slave_visual(e, t, render); */

/* 	if(render) { */
/* 		return; */
/* 	} */

/* 	if(e->args[2] && !(t % 5)) { */
/* 		cmplx offset = (frand()-0.5)*30 + (frand()-0.5)*20.0*I; */
/* 		PARTICLE( */
/* 			.sprite = "smoothdot", */
/* 			.pos = offset, */
/* 			.color = e->args[1] ? RGBA(1.0, 0.5, 0.0, 0.0) : RGBA(0.0, 0.5, 0.5, 0.0), */
/* 			.draw_rule = Shrink, */
/* 			.rule = enemy_flare, */
/* 			.timeout = 50, */
/* 			.args = { */
/* 				(-50.0*I-offset)/50.0, */
/* 				add_ref(e) */
/* 			}, */
/* 		); */
/* 	} */
/* } */

/* static int iku_overload_slave(Enemy *e, int t) { */
/* 	GO_TO(e, e->args[0], 0.05); */

/* 	if(e->args[1]) { */
/* 		if(creal(e->args[1]) < 2) { */
/* 			e->args[1] += 1; */
/* 			return 0; */
/* 		} */

/* 		if(global.frames == creal(e->args[3])) { */
/* 			cmplx o2 = e->args[2]; */
/* 			e->args[2] = 0; */
/* 			Enemy *new = iku_overload_find_next_slave(e->pos, 75); */
/* 			e->args[2] = o2; */

/* 			if(new && e != new) { */
/* 				e->args[1] = 0; */
/* 				e->args[2] = new->args[2] = 600; */
/* 				new->args[1] = 1; */
/* 				new->args[3] = global.frames + 55 - 5 * global.diff; */

/* 				Laser *l = create_laserline_ab(e->pos, new->pos, 10, 30, e->args[2], RGBA(0.3, 1, 1, 0)); */
/* 				l->ent.draw_layer = LAYER_LASER_LOW; */
/* 				l->unclearable = true; */

/* 				if(global.diff > D_Easy) { */
/* 					int cnt = floor(global.diff * 2.5), i; */
/* 					double r = frand() * 2 * M_PI; */

/* 					for(i = 0; i < cnt; ++i) { */
/* 						PROJECTILE( */
/* 							.proto = pp_rice, */
/* 							.pos = e->pos, */
/* 							.color = RGBA(1, 1, 0, 0), */
/* 							.rule = asymptotic, */
/* 							.args = { 2*cexp(I*(r+i*2*M_PI/cnt)), 2 }, */
/* 						); */
/* 					} */

/* 					play_sound("shot2"); */
/* 				} */

/* 				play_sound("redirect"); */
/* 			} else { */
/* 				Enemy *o; */
/* 				Laser *l; */
/* 				int cnt = 6 + 2 * global.diff, i; */

/* 				e->args[2] = 1; */

/* 				for(o = global.enemies.first; o; o = o->next) { */
/* 					if(!o->args[2]) */
/* 						continue; */

/* 					for(i = 0; i < cnt; ++i) { */
/* 						PROJECTILE( */
/* 							.proto = pp_ball, */
/* 							.pos = o->pos, */
/* 							.color = RGBA(0, 1, 1, 0), */
/* 							.rule = asymptotic, */
/* 							.args = { 1.5*cexp(I*(t + i*2*M_PI/cnt)), 8}, */
/* 						); */
/* 					} */

/* 					o->args[1] = 0; */
/* 					o->args[2] = 0; */

/* 					stage_shake_view(5); */
/* 				} */

/* 				for(l = global.lasers.first; l; l = l->next) { */
/* 					l->deathtime = global.frames - l->birthtime + 20; */
/* 				} */
/* 				play_sound("boom"); */
/* 				iku_overload_fire_trigger_bullet(); */
/* 			} */
/* 		} */
/* 	} */

/* 	if(e->args[2]) { */
/* 		e->args[2] -= 1; */
/* 	} */

/* 	return 0; */
/* } */

/* static void iku_overload_slave_visual(EntityInterface *e) { */

TASK(iku_overload_slave_visual, { BoxedIkuSlave slave; }) {


	/* if(e->args[2] && !(t % 5)) { */
	/* 	cmplx offset = (frand()-0.5)*30 + (frand()-0.5)*20.0*I; */
	/* 	PARTICLE( */
	/* 		.sprite = "smoothdot", */
	/* 		.pos = offset, */
	/* 		.color = e->args[1] ? RGBA(1.0, 0.5, 0.0, 0.0) : RGBA(0.0, 0.5, 0.5, 0.0), */
	/* 		.draw_rule = Shrink, */
	/* 		.rule = enemy_flare, */
	/* 		.timeout = 50, */
	/* 		.args = { */
	/* 			(-50.0*I-offset)/50.0, */
	/* 			add_ref(e) */
	/* 		}, */
	/* 	); */
	/* } */
}

DEFINE_EXTERN_TASK(stage5_spell_overload) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_towards(VIEWPORT_W/2 + 50.0 * I, 0.02);
	BEGIN_BOSS_ATTACK(&ARGS);

	int count = 5;
	double step = VIEWPORT_W / (double)count;
	for(;;) {
		for(int i = 0; i < count; ++i) {
			for(int j = 0; j < count; ++j) {
				cmplx epos = step * (0.5 + i) + (step * j + 125) * I;
				IkuSlave *slave = stage5_midboss_slave(epos);
				INVOKE_SUBTASK(iku_overload_slave_visual, ENT_BOX(slave));
				// 0 = pos
				// 1 = colour (iku_slave_visual)
				// 2 = laserline pos
				// 3 = determines if "first frame"
				//create_enemy4c(boss->pos, ENEMY_IMMUNE, iku_overload_slave_visual, iku_overload_slave, epos, 0, 0, 1);
			}
		}
		WAIT(200);
		//iku_overload_fire_trigger_bullet();
	}

}
