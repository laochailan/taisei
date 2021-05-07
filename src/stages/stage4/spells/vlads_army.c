/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"
#include "../kurumi.h"
#include "enemy_classes.h"

#include "global.h"

static void kurumi_extra_swirl_visual(Enemy *e, int time, bool render) {
	if(!render) {
		// why the hell was this here?
		// Swirl(e, time, render);
		return;
	}

	// FIXME: blend
	r_blend(BLEND_SUB);
	Swirl(e, time, render);
	r_blend(BLEND_PREMUL_ALPHA);
}

static void kurumi_extra_fairy_visual(Enemy *e, int time, bool render) {
	if(!render) {
		Fairy(e, time, render);
		return;
	}

	// r_blend(BLEND_ADD);
	r_shader("sprite_negative");
	Fairy(e, time, render);
	r_shader("sprite_default");
	// r_blend(BLEND_ALPHA);
}

static void kurumi_extra_bigfairy_visual(Enemy *e, int time, bool render) {
	if(!render) {
		BigFairy(e, time, render);
		return;
	}

	// r_blend(BLEND_ADD);
	r_shader("sprite_negative");
	BigFairy(e, time, render);
	r_shader("sprite_default");
	// r_blend(BLEND_ALPHA);
}


TASK(kurumi_vladsarmy_shield_death_proj, { cmplx pos; MoveParams move; }) {
	Projectile *p = TASK_BIND(PROJECTILE(
		.proto = pp_ball,
		.pos = ARGS.pos,
		.move = ARGS.move
	));

	int duration = 60;
	for(int i = 0; i < duration; i++, YIELD) {
		p->color = *color_lerp(
			RGBA(2.0, 0.0, 0.0, 0.0),
			RGBA(0.2, 0.1, 0.5, 0.0),
		i / (float) duration);
	}
}

TASK(kurumi_vladsarmy_shield_death, { BoxedEnemy enemy;  }) {
	Enemy *e = ENT_UNBOX(ARGS.enemy);
	if(e == NULL || !(e->flags & EFLAG_IMPENETRABLE)) {
		return;
	}
	cmplx pos = e->pos;

	play_sfx_ex("boon", 3, false);
	int shots = 5;
	int count = 10;

	for(int i = 0; i < shots; i++) {

		for(int j = 0; j < count; ++j) {
			cmplx dir = cdir(M_TAU/count * j);
			real speed = 2.5 * (1 + rng_real());

			INVOKE_TASK(kurumi_vladsarmy_shield_death_proj,
				.pos = pos,
				.move = move_asymptotic_simple(speed * dir, -0.7)
			);
		}

		WAIT(6);
	}
}

TASK(kurumi_vladsarmy_shield_manage, { BoxedEnemy enemy; BoxedBoss boss; real angle; int timeout; }) {
	Enemy *e = TASK_BIND(ARGS.enemy);

	for(int i = 0; i < ARGS.timeout; i++, YIELD) {
		e->pos = NOT_NULL(ENT_UNBOX(ARGS.boss))->pos + 140 * cdir(ARGS.angle + 0.1 * i); 
	}
	e->flags &= ~EFLAG_IMPENETRABLE;
	enemy_kill(e);
}

TASK(kurumi_vladsarmy_shield, { BoxedBoss boss; real angle; }) {
	int hp = 500;
	Boss *b = NOT_NULL(ENT_UNBOX(ARGS.boss));
	
	Enemy *e = create_enemy2c(b->pos, hp, kurumi_extra_swirl_visual, NULL, 0, 0);
	e->flags = EFLAG_IMPENETRABLE;

	int timeout = 800;
	INVOKE_SUBTASK_WHEN(&e->events.killed, kurumi_vladsarmy_shield_death, ENT_BOX(e));
	INVOKE_SUBTASK(kurumi_vladsarmy_shield_manage,
		.enemy=ENT_BOX(e),
		.boss = ARGS.boss,
		.angle = ARGS.angle,
		.timeout = timeout
	);
	STALL;
}

TASK(kurumi_vladsarmy_bigfairy, { cmplx pos; cmplx target; }) {
	Enemy *e = TASK_BIND(espawn_big_fairy(ARGS.pos, ITEMS(.points = 2, .power = 1)));
	e->visual_rule = kurumi_extra_bigfairy_visual;
	
	int escapetime = difficulty_value(400, 400, 400, 4000);

	e->move = move_towards(ARGS.target, 0.02);

	WAIT(50);
	for(int k = 0; k < escapetime; k += WAIT(60)) { 
		int count = 5;
		cmplx phase = rng_dir();
		for(int j = 0; j < count; j++) {
			cmplx dir = cdir(M_TAU * j / count);
			if(global.diff == D_Lunatic)
				dir *= phase;
			create_lasercurve2c(e->pos, 20, 200, RGBA(1.0, 0.3, 0.7, 0.0), las_accel, dir, 0.1 * dir);
			PROJECTILE(
				.proto = pp_bullet,
				.pos = e->pos,
				.color = RGB(1.0, 0.3, 0.7),
				.move = move_accelerated(dir, 0.1 * dir)
			);
		}

		// XXX make this sound enjoyable
		play_sfx_ex("laser1", 60, false);
	}

	e->move = move_towards(ARGS.target - I * VIEWPORT_H, 0.01);
}

DEPRECATED_DRAW_RULE
static void kurumi_extra_drainer_draw(Projectile *p, int time, ProjDrawRuleArgs args) {
	cmplx org = p->pos;
	cmplx targ = p->args[1];
	double a = 0.5 * creal(p->args[2]);
	Texture *tex = res_texture("part/sinewave");

	r_shader_standard();

	r_color4(1.0 * a, 0.5 * a, 0.5 * a, 0);
	loop_tex_line_p(org, targ, 16, time * 0.01, tex);

	r_color4(0.4 * a, 0.2 * a, 0.2 * a, 0);
	loop_tex_line_p(org, targ, 18, time * 0.0043, tex);

	r_color4(0.5 * a, 0.2 * a, 0.5 * a, 0);
	loop_tex_line_p(org, targ, 24, time * 0.003, tex);
}

TASK(kurumi_vladsarmy_drainer, { BoxedBoss boss; BoxedEnemy enemy; }) {
	Projectile *p = TASK_BIND(PROJECTILE(
		.size = 1+I,
		.pos = NOT_NULL(ENT_UNBOX(ARGS.enemy))->pos,
		.draw_rule = kurumi_extra_drainer_draw,
		.shader = "sprite_default",
		.flags = PFLAG_NOCLEAR | PFLAG_NOCOLLISION,
	));

	for(int i = 0;; i++, YIELD) {
		p->pos = global.boss->pos;
		Enemy *e = ENT_UNBOX(ARGS.enemy);
		Boss *boss = ENT_UNBOX(ARGS.boss);

		if(e && boss) {
			p->args[1] = e->pos;
			p->args[2] = approach(p->args[2], 1, 0.5);

			if(i > 40 && e->hp > 0) {
				// TODO: maybe add a special sound for this?

				float drain = fmin(4, e->hp);
				ent_damage(&e->ent, &(DamageInfo) { .amount = drain });
				boss->current->hp = fmin(boss->current->maxhp, boss->current->hp + drain * 2);
			}
		} else {
			p->args[2] = approach(p->args[2], 0, 0.5);
			if(!creal(p->args[2])) {
				kill_projectile(p);
			}
		}
	}
}


TASK(kurumi_vladsarmy_fairy, { cmplx start_pos; cmplx target_pos; int attack_time; int chase_time; int attack_type; BoxedBoss boss; }){
	Enemy *e = TASK_BIND(espawn_fairy_blue(ARGS.start_pos, ITEMS(.points = 1)));
	e->visual_rule = kurumi_extra_fairy_visual;
	e->flags |= EFLAG_NO_AUTOKILL;

	e->move = move_towards(ARGS.target_pos, 0.1);

	WAIT(50);
		e->flags &= ~EFLAG_NO_AUTOKILL;

	WAIT(ARGS.attack_time-70);

	real speed = difficulty_value(2.0, 2.1, 2.2, 2.3);

	for(int k = 0; k <= 2; k++, WAIT(20)) {
		play_sfx_loop("shot1_loop");
		cmplx vel = speed * rng_dir();
		if(ARGS.attack_type == 0) {
			int corners = 5;
			real len = 50;
			int count = 4;
			for(int i = 0; i < corners; i++) {
				for(int j = 0; j < count; j++) {
					cmplx offset = 0.5 / tan(M_TAU / corners) * I + (j / (real)count - 0.5);
					offset *= len*cdir(M_TAU / corners * i);
					PROJECTILE(
						.proto = pp_flea,
						.pos = e->pos + offset,
						.color = RGB(1, 0.3, 0.5),
						.move = move_linear(vel + 0.1 * cnormalize(offset)),
					);
				}
			}
		} else {
			int count = 30;
			double rad = 20;
			for(int j = 0; j < count; j++) {
				real x = M_TAU * (j / (real)count-0.5);
				cmplx pos = 0.5 * cos(x) + sin(2 * x) + (0.5 * sin(x) + cos( 2 * x))*I;
				pos *= cnormalize(vel);
				PROJECTILE(
					.proto = pp_flea,
					.pos = e->pos+rad*pos,
					.color = RGB(0.5, 0.3, 1),
					.move = move_linear(vel + 0.1*pos),
				);
			}
		}
	}
	e->move = move_linear((global.plr.pos - e->pos) / ARGS.chase_time);
	INVOKE_TASK(kurumi_vladsarmy_drainer, ARGS.boss, ENT_BOX(e));
	play_sfx("redirect");

	int chase_drop_step = difficulty_value(17, 14, 11, 8);
	for(int i = 0; i < ARGS.chase_time / chase_drop_step; i++, WAIT(chase_drop_step)) {
		if(global.diff>D_Easy) {
			Color *clr = RGBA_MUL_ALPHA(0.1 + 0.07 * i, 0.3, 1 - 0.05 * i, 0.8);
			clr->a = 0;

			PROJECTILE(
				.proto = pp_ball,
				.pos = e->pos,
				.color = clr,
				.timeout = 50,
			);
		}
	}
	e->move = move_towards(global.boss->pos, 0.04);
}

DEFINE_EXTERN_TASK(kurumi_vladsarmy) {
	Boss *b = INIT_BOSS_ATTACK();
	BEGIN_BOSS_ATTACK();

	cmplx startpos = VIEWPORT_W * 0.5 + VIEWPORT_H * 0.28 * I;

	int prey_count = 20;
	int fairy_chase_time = difficulty_value(105, 100, 95, 90);
	
	for(int run = 0;; run++) {
		b->move = move_towards(startpos, 0.01);
		int direction = run&1;

		int castlimit = b->current->maxhp * 0.05;
		int shieldlimit = b->current->maxhp * 0.1;


		if(b->current->hp >= shieldlimit) {
			int shield_count = 12;
			for(int i = 0; i < shield_count; ++i) {
				real angle = M_TAU/shield_count * i;
				INVOKE_SUBTASK(kurumi_vladsarmy_shield, ENT_BOX(b), .angle = angle);
			}
		}

		WAIT(90);
		for(int i = 0; i < prey_count; i++) {
			b->current->hp -= 600;
			if(b->current->hp < castlimit) {
				b->current->hp = castlimit;
			}

			RNG_ARRAY(rand, 2);
			cmplx pos = vrng_real(rand[0]) * VIEWPORT_W / 2 + I * vrng_real(rand[1]) * VIEWPORT_H * 0.67;
			
			if(direction)
				pos = VIEWPORT_W-creal(pos)+I*cimag(pos);

			INVOKE_SUBTASK(kurumi_vladsarmy_fairy,
				       .start_pos = b->pos - 300 * (1 - 2 * direction),
				       .target_pos = pos,
				       .attack_time = 100 + 20 * i,
				       .chase_time = fairy_chase_time,
				       .attack_type = direction,
				       .boss = ENT_BOX(b)
			);
		}

		// XXX: maybe add a special sound for this?
		play_sfx("shot_special1");

		cmplx sidepos = VIEWPORT_W * (0.5 + 0.3 * (1 - 2 * direction)) + VIEWPORT_H * 0.28 * I;
		b->move = move_towards(sidepos, 0.1);
		WAIT(100);
		b->move = move_towards(sidepos + 30 * I, 0.1);

		aniplayer_queue_frames(&b->ani, "muda", 110);
		aniplayer_queue(&b->ani, "main", 0);

		WAIT(110);

		b->move = move_towards(startpos, 0.1); 

		if(global.diff >= D_Hard) {
			double offset = VIEWPORT_W * 0.5;
			cmplx pos = 0.5 * VIEWPORT_W + I * (VIEWPORT_H - 100);
			cmplx target = 0.5 * VIEWPORT_W + VIEWPORT_H * 0.3 * I;

			INVOKE_SUBTASK(kurumi_vladsarmy_bigfairy,
				.pos = pos + offset,
				.target = target + 0.8*offset
			);
			INVOKE_SUBTASK(kurumi_vladsarmy_bigfairy,
				.pos = pos - offset,
				.target = target - 0.8*offset
			);
		}
		WAIT(100);

		/*if((t == length-20 && global.diff == D_Easy)|| b->current->hp < shieldlimit) {
			for(Enemy *e = global.enemies.first; e; e = e->next) {
				if(e->logic_rule == kurumi_extra_shield) {
					e->args[2] = 1; // discharge extra shield
					e->hp = 0;
					continue;
				}
			}
		}*/
	}
}
