/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "stage1_events.h"
#include "global.h"
#include "stagetext.h"
#include "common_tasks.h"

static Dialog *stage1_dialog_pre_boss(void) {
	PlayerMode *pm = global.plr.mode;
	Dialog *d = dialog_create();
	dialog_set_char(d, DIALOG_LEFT, pm->character->lower_name, "normal", NULL);
	dialog_set_char(d, DIALOG_RIGHT, "cirno", "normal", NULL);
	pm->dialog->stage1_pre_boss(d);
	dialog_add_action(d, &(DialogAction) { .type = DIALOG_SET_BGM, .data = "stage1boss"});
	return d;
}

static Dialog *stage1_dialog_post_boss(void) {
	PlayerMode *pm = global.plr.mode;
	Dialog *d = dialog_create();
	dialog_set_char(d, DIALOG_LEFT, pm->character->lower_name, "normal", NULL);
	dialog_set_char(d, DIALOG_RIGHT, "cirno", "defeated", "defeated");
	pm->dialog->stage1_post_boss(d);
	return d;
}

static void cirno_intro(Boss *c, int time) {
	if(time < 0)
		return;

	GO_TO(c, VIEWPORT_W/2.0 + 100.0*I, 0.035);
}

static int cirno_snowflake_proj(Projectile *p, int time) {
	if(time < 0)
		return ACTION_ACK;

	int split_time = 200 - 20*global.diff - creal(p->args[1]) * 3;

	if(time < split_time) {
		p->pos += p->args[0];
	} else {
		if(time == split_time) {
			play_sound_ex("redirect", 30, false);
			play_sound_ex("shot_special1", 30, false);
			color_lerp(&p->color, RGB(0.5, 0.5, 0.5), 0.5);
			spawn_projectile_highlight_effect(p);
		}

		p->pos -= cabs(p->args[0]) * cexp(I*p->angle);
	}

	return 1;
}

static void cirno_icy(Boss *b, int time) {
	int interval = 70 - 8 * global.diff;
	int t = time % interval;
	int run = time / interval;
	int size = 5+3*sin(337*run);

	TIMER(&t);

	if(time < 0) {
		return;
	}

	cmplx vel = (1+0.125*global.diff)*cexp(I*fmod(200*run,M_PI));
	int c = 6;
	double dr = 15;

	FROM_TO_SND("shot1_loop", 0, 3*size, 3) {
		for(int i = 0; i < c; i++) {
			double ang = 2*M_PI/c*i+run*515;
			cmplx phase = cexp(I*ang);
			cmplx pos = b->pos+vel*t+dr*_i*phase;

			PROJECTILE(
				.proto = pp_crystal,
				.pos = pos+6*I*phase,
				.color = RGB(0.0, 0.1 + 0.1 * size / 5, 0.8),
				.rule = cirno_snowflake_proj,
				.move = move_linear(vel),
				.angle = ang+M_PI/4,
				.max_viewport_dist = 64,
			);


			PROJECTILE(
				.proto = pp_crystal,
				.pos = pos-6*I*phase,
				.color = RGB(0.0,0.1+0.1*size/5,0.8),
				.rule = cirno_snowflake_proj,
				.args = { vel, _i },
				.angle = ang-M_PI/4,
				.max_viewport_dist = 64,
			);

			int split = 3;

			if(_i > split) {
				cmplx pos0 = b->pos+vel*t+dr*split*phase;

				for(int j = -1; j <= 1; j+=2) {
					cmplx phase2 = cexp(I*M_PI/4*j)*phase;
					cmplx pos2 = pos0+(dr*(_i-split))*phase2;

					PROJECTILE(
						.proto = pp_crystal,
						.pos = pos2,
						.color = RGB(0.0,0.3*size/5,1),
						.rule = cirno_snowflake_proj,
						.args = { vel, _i },
						.angle = ang+M_PI/4*j,
						.max_viewport_dist = 64,
					);
				}
			}
		}
	}
}

static Projectile* spawn_stain(cmplx pos, float angle, int to) {
	return PARTICLE(
		.sprite = "stain",
		.pos = pos,
		.draw_rule = ScaleFade,
		.timeout = to,
		.angle = angle,
		.color = RGBA(0.4, 0.4, 0.4, 0),
		.args = {0, 0, 0.8*I}
	);
}

static int cirno_pfreeze_frogs(Projectile *p, int t) {
	if(t < 0)
		return ACTION_ACK;

	Boss *parent = global.boss;

	if(parent == NULL)
		return ACTION_DESTROY;

	int boss_t = (global.frames - parent->current->starttime) % 320;

	if(boss_t < 110)
		linear(p, t);
	else if(boss_t == 110) {
		p->color = *RGB(0.7, 0.7, 0.7);
		spawn_stain(p->pos, p->angle, 30);
		spawn_stain(p->pos, p->angle, 30);
		spawn_projectile_highlight_effect(p);
		play_sound("shot_special1");
	}

	if(t == 240) {
		p->prevpos = p->pos;
		p->pos0 = p->pos;
		p->args[0] = (1.8+0.2*global.diff)*cexp(I*2*M_PI*frand());
		spawn_stain(p->pos, p->angle, 30);
		spawn_projectile_highlight_effect(p);
		play_sound_ex("shot2", 0, false);
	}

	if(t > 240)
		linear(p, t-240);

	return 1;
}

void cirno_perfect_freeze(Boss *c, int time) {
	int t = time % 320;
	TIMER(&t);

	if(time < 0)
		return;

	FROM_TO(-40, 0, 1)
		GO_TO(c, VIEWPORT_W/2.0 + 100.0*I, 0.04);

	FROM_TO_SND("shot1_loop",20,80,1) {
		float r = frand();
		float g = frand();
		float b = frand();

		int i;
		int n = global.diff;
		for(i = 0; i < n; i++) {
			PROJECTILE(
				.proto = pp_ball,
				.pos = c->pos,
				.color = RGB(r, g, b),
				.rule = cirno_pfreeze_frogs,
				.args = { 4*cexp(I*tsrand()) },
			);
		}
	}

	GO_AT(c, 160, 190, 2 + 1.0*I);

	int d = max(0, global.diff - D_Normal);
	AT(140-50*d)
		aniplayer_queue(&c->ani,"(9)",0);
	AT(220+30*d)
		aniplayer_queue(&c->ani,"main",0);
	FROM_TO_SND("shot1_loop", 160 - 50*d, 220 + 30*d, 6-global.diff/2) {
		float r1, r2;

		if(global.diff > D_Normal) {
			r1 = sin(time/M_PI*5.3) * cos(2*time/M_PI*5.3);
			r2 = cos(time/M_PI*5.3) * sin(2*time/M_PI*5.3);
		} else {
			r1 = nfrand();
			r2 = nfrand();
		}

		PROJECTILE(
			.proto = pp_rice,
			.pos = c->pos + 60,
			.color = RGB(0.3, 0.4, 0.9),
			.rule = asymptotic,
			.args = { (2.+0.2*global.diff)*cexp(I*(carg(global.plr.pos - c->pos) + 0.5*r1)), 2.5 }
		);
		PROJECTILE(
			.proto = pp_rice,
			.pos = c->pos - 60,
			.color = RGB(0.3, 0.4, 0.9),
			.rule = asymptotic,
			.args = { (2.+0.2*global.diff)*cexp(I*(carg(global.plr.pos - c->pos) + 0.5*r2)), 2.5 }
		);
	}

	GO_AT(c, 190, 220, -2);

	FROM_TO(280, 320, 1)
		GO_TO(c, VIEWPORT_W/2.0 + 100.0*I, 0.04);
}

void cirno_pfreeze_bg(Boss *c, int time) {
	r_color4(0.5, 0.5, 0.5, 1.0);
	fill_viewport(time/700.0, time/700.0, 1, "stage1/cirnobg");
	r_blend(BLEND_MOD);
	r_color4(0.7, 0.7, 0.7, 0.5);
	fill_viewport(-time/700.0 + 0.5, time/700.0+0.5, 0.4, "stage1/cirnobg");
	r_blend(BLEND_PREMUL_ALPHA);
	r_color4(0.35, 0.35, 0.35, 0.0);
	fill_viewport(0, -time/100.0, 0, "stage1/snowlayer");
	r_color4(1.0, 1.0, 1.0, 1.0);
}

static void cirno_mid_flee(Boss *c, int time) {
	if(time >= 0) {
		GO_TO(c, -250 + 30 * I, 0.02)
	}
}

Boss* stage1_spawn_cirno(cmplx pos) {
	Boss *cirno = create_boss("Cirno", "cirno", pos);
	boss_set_portrait(cirno, get_sprite("dialog/cirno"), get_sprite("dialog/cirno_face_normal"));
	cirno->shadowcolor = *RGBA_MUL_ALPHA(0.6, 0.7, 1.0, 0.25);
	cirno->glowcolor = *RGB(0.2, 0.35, 0.5);
	return cirno;
}

static Boss* create_cirno_mid(void) {
	Boss *cirno = stage1_spawn_cirno(VIEWPORT_W + 220 + 30.0*I);

	boss_add_attack(cirno, AT_Move, "Introduction", 2, 0, cirno_intro, NULL);
	boss_add_attack(cirno, AT_Normal, "Icy Storm", 20, 24000, cirno_icy, NULL);
	boss_add_attack_from_info(cirno, &stage1_spells.mid.perfect_freeze, false);
	boss_add_attack(cirno, AT_Move, "Flee", 5, 0, cirno_mid_flee, NULL);

	boss_start_attack(cirno, cirno->attacks);
	return cirno;
}

static void cirno_intro_boss(Boss *c, int time) {
	if(time < 0)
		return;
	TIMER(&time);
	GO_TO(c, VIEWPORT_W/2.0 + 100.0*I, 0.05);

	AT(120)
		global.dialog = stage1_dialog_pre_boss();
}

static void cirno_iceplosion0(Boss *c, int time) {
	int t = time % 300;
	TIMER(&t);

	if(time < 0)
		return;

	AT(20) {
		aniplayer_queue(&c->ani,"(9)",1);
		aniplayer_queue(&c->ani,"main",0);
		play_sound("shot_special1");
	}

	FROM_TO(20,30,2) {
		int i;
		int n = 8+global.diff;
		for(i = 0; i < n; i++) {
			PROJECTILE(
				.proto = pp_plainball,
				.pos = c->pos,
				.color = RGB(0,0,0.5),
				.rule = asymptotic,
				.args = { (3+_i/3.0)*cexp(I*(2*M_PI/n*i + carg(global.plr.pos-c->pos))), _i*0.7 }
			);
		}
	}

	FROM_TO_SND("shot1_loop",40,100,1+2*(global.diff<D_Hard)) {
		PROJECTILE(
			.proto = pp_crystal,
			.pos = c->pos,
			.color = RGB(0.3,0.3,0.8),
			.rule = accelerated,
			.args = { global.diff/4.*cexp(2.0*I*M_PI*frand()) + 2.0*I, 0.002*cexp(I*(M_PI/10.0*(_i%20))) }
		);
	}

	FROM_TO(150, 300, 30-5*global.diff) {
		float dif = M_PI*2*frand();
		int i;
		play_sound("shot1");
		for(i = 0; i < 20; i++) {
			PROJECTILE(
				.proto = pp_plainball,
				.pos = c->pos,
				.color = RGB(0.04*_i,0.04*_i,0.4+0.04*_i),
				.rule = asymptotic,
				.args = { (3+_i/4.0)*cexp(I*(2*M_PI/8.0*i + dif)), 2.5 }
			);
		}
	}
}

void cirno_crystal_rain(Boss *c, int time) {
	int t = time % 500;
	TIMER(&t);

	if(time < 0)
		return;

	// PLAY_FOR("shot1_loop",0,499);

	if(!(time % 10)) {
		play_sound("shot2");
	}

	int hdiff = max(0, (int)global.diff - D_Normal);

	if(frand() > 0.95-0.1*global.diff) {
		tsrand_fill(2);
		PROJECTILE(
			.proto = pp_crystal,
			.pos = VIEWPORT_W*afrand(0),
			.color = RGB(0.2,0.2,0.4),
			.rule = accelerated,
			.args = { 1.0*I, 0.01*I + (-0.005+0.005*global.diff)*anfrand(1) }
		);
	}

	AT(100)
		aniplayer_queue(&c->ani,"(9)",0);
	AT(400)
		aniplayer_queue(&c->ani,"main",0);
	FROM_TO(100, 400, 120-20*global.diff - 10 * hdiff) {
		float i;
		bool odd = (hdiff? (_i&1) : 0);
		float n = (global.diff-1+hdiff*4 + odd)/2.0;

		play_sound("shot_special1");
		for(i = -n; i <= n; i++) {
			PROJECTILE(
				.proto = odd? pp_plainball : pp_bigball,
				.pos = c->pos,
				.color = RGB(0.2,0.2,0.9),
				.rule = asymptotic,
				.args = { 2*cexp(I*carg(global.plr.pos-c->pos)+0.3*I*i), 2.3 }
			);
		}
	}

	GO_AT(c, 20, 70, 1+0.6*I);
	GO_AT(c, 120, 170, -1+0.2*I);
	GO_AT(c, 230, 300, -1+0.6*I);

	FROM_TO(400, 500, 1)
		GO_TO(c, VIEWPORT_W/2.0 + 100.0*I, 0.01);
}

static void cirno_iceplosion1(Boss *c, int time) {
	int t = time % 300;
	TIMER(&t);

	if(time < 0)
		GO_TO(c, VIEWPORT_W/2.0 + 100.0*I, 0.02);

	AT(20) {
		play_sound("shot_special1");
	}
	AT(20)
		aniplayer_queue(&c->ani,"(9)",0);
	AT(30)
		aniplayer_queue(&c->ani,"main",0);

	FROM_TO(20,30,2) {
		int i;
		for(i = 0; i < 15+global.diff; i++) {
			PROJECTILE(
				.proto = pp_plainball,
				.pos = c->pos,
				.color = RGB(0,0,0.5),
				.rule = asymptotic,
				.args = { (3+_i/3.0)*cexp(I*((2)*M_PI/8.0*i + (0.1+0.03*global.diff)*(1 - 2*frand()))), _i*0.7 }
			);
		}
	}

	FROM_TO_SND("shot1_loop",40,100,2+2*(global.diff<D_Hard)) {
		for(int i = 1; i >= -1; i -= 2) {
			PROJECTILE(
				.proto = pp_crystal,
				.pos = c->pos + i * 100,
				.color = RGB(0.3,0.3,0.8),
				.rule = accelerated,
				.args = {
					1.5*cexp(2.0*I*M_PI*frand()) - i * 0.4 + 2.0*I*global.diff/4.0,
					0.002*cexp(I*(M_PI/10.0*(_i%20)))
				}
			);
		}
	}

	FROM_TO(150, 300, 30 - 6 * global.diff) {
		float dif = M_PI*2*frand();
		int i;

		if(_i > 15) {
			_i = 15;
		}

		play_sound("shot1");
		for(i = 0; i < 20; i++) {
			PROJECTILE(
				.proto = pp_plainball,
				.pos = c->pos,
				.color = RGB(0.04*_i,0.04*_i,0.4+0.04*_i),
				.rule = asymptotic,
				.args = { (3+_i/3.0)*cexp(I*(2*M_PI/8.0*i + dif)), 2.5 }
			);
		}
	}
}

static Color* halation_color(Color *out_clr, float phase) {
	if(phase < 0.5) {
		*out_clr = *color_lerp(
			RGB(0.4, 0.4, 0.75),
			RGB(0.4, 0.4, 0.3),
			phase * phase
		);
	} else {
		*out_clr = *color_lerp(
			RGB(0.4, 0.4, 0.3),
			RGB(1.0, 0.3, 0.2),
			(phase - 0.5) * 2
		);
	}

	return out_clr;
}

static void halation_laser(Laser *l, int time) {
	static_laser(l, time);

	if(time >= 0) {
		halation_color(&l->color, l->width / cimag(l->args[1]));
		l->color.a = 0;
	}
}

static cmplx halation_calc_orb_pos(cmplx center, float rotation, int proj, int projs) {
	double f = (double)((proj % projs)+0.5)/projs;
	return 200 * cexp(I*(rotation + f * 2 * M_PI)) + center;
}

static int halation_orb(Projectile *p, int time) {
	if(time < 0) {
		return ACTION_ACK;
	}

	if(!(time % 4)) {
		spawn_stain(p->pos, global.frames * 15, 20);
	}

	cmplx center = p->args[0];
	double rotation = p->args[1];
	int id = creal(p->args[2]);
	int count = cimag(p->args[2]);
	int halate_time = creal(p->args[3]);
	int phase_time = 60;

	cmplx pos0 = halation_calc_orb_pos(center, rotation, id, count);
	cmplx pos1 = halation_calc_orb_pos(center, rotation, id + count/2, count);
	cmplx pos2 = halation_calc_orb_pos(center, rotation, id + count/2 - 1, count);
	cmplx pos3 = halation_calc_orb_pos(center, rotation, id + count/2 - 2, count);

	GO_TO(p, pos2, 0.1);

	if(p->type == PROJ_DEAD) {
		return 1;
	}

	if(time == halate_time) {
		create_laserline_ab(pos2, pos3, 15, phase_time * 0.5, phase_time * 2.0, &p->color);
		create_laserline_ab(pos0, pos2, 15, phase_time, phase_time * 1.5, &p->color)->lrule = halation_laser;
	} if(time == halate_time + phase_time * 0.5) {
		play_sound("laser1");
	} else if(time == halate_time + phase_time) {
		play_sound("shot1");
		create_laserline_ab(pos0, pos1, 12, phase_time, phase_time * 1.5, &p->color)->lrule = halation_laser;
	} else if(time == halate_time + phase_time * 2) {
		play_sound("shot1");
		create_laserline_ab(pos0, pos3, 15, phase_time, phase_time * 1.5, &p->color)->lrule = halation_laser;
		create_laserline_ab(pos1, pos3, 15, phase_time, phase_time * 1.5, &p->color)->lrule = halation_laser;
	} else if(time == halate_time + phase_time * 3) {
		play_sound("shot1");
		create_laserline_ab(pos0, pos1, 12, phase_time, phase_time * 1.5, &p->color)->lrule = halation_laser;
		create_laserline_ab(pos0, pos2, 15, phase_time, phase_time * 1.5, &p->color)->lrule = halation_laser;
	} else if(time == halate_time + phase_time * 4) {
		play_sound("shot1");
		play_sound("shot_special1");

		Color colors[] = {
			// i *will* revert your commit if you change this, no questions asked.
			{ 226/255.0, 115/255.0,  45/255.0, 1 },
			{  54/255.0, 179/255.0, 221/255.0, 1 },
			{ 140/255.0, 147/255.0, 149/255.0, 1 },
			{  22/255.0,  96/255.0, 165/255.0, 1 },
			{ 241/255.0, 197/255.0,  31/255.0, 1 },
			{ 204/255.0,  53/255.0,  84/255.0, 1 },
			{ 116/255.0,  71/255.0, 145/255.0, 1 },
			{  84/255.0, 171/255.0,  72/255.0, 1 },
			{ 213/255.0,  78/255.0, 141/255.0, 1 },
		};

		int pcount = sizeof(colors)/sizeof(Color);
		float rot = frand() * 2 * M_PI;

		for(int i = 0; i < pcount; ++i) {
			PROJECTILE(
				.proto = pp_crystal,
				.pos = p->pos,
				.color = colors+i,
				.rule = asymptotic,
				.args = { cexp(I*(rot + M_PI * 2 * (float)(i+1)/pcount)), 3 }
			);
		}

		return ACTION_DESTROY;
	}

	return 1;
}

void cirno_snow_halation(Boss *c, int time) {
	int t = time % 300;
	TIMER(&t);

	// TODO: get rid of the "static" nonsense already! #ArgsForBossAttacks2017
	// tfw it's 2018 and still no args
	// tfw when you then add another static
	static cmplx center;
	static float rotation;
	static int cheater;

	if(time == EVENT_BIRTH)
		cheater = 0;

	if(time < 0) {
		return;
	}

	if(cheater >= 8) {
		GO_TO(c, global.plr.pos,0.05);
		aniplayer_queue(&c->ani,"(9)",0);
	} else {
		GO_TO(c, VIEWPORT_W/2.0+100.0*I, 0.05);
	}

	AT(60) {
		center = global.plr.pos;
		rotation = (M_PI/2.0) * (1 + time / 300);
		aniplayer_queue(&c->ani,"(9)",0);
	}

	const int interval = 3;
	const int projs = 10 + 4 * (global.diff - D_Hard);

	FROM_TO_SND("shot1_loop", 60, 60 + interval * (projs/2 - 1), interval) {
		int halate_time = 35 - _i * interval;

		for(int p = _i*2; p <= _i*2 + 1; ++p) {
			Color clr;
			halation_color(&clr, 0);
			clr.a = 0;

			PROJECTILE(
				.proto = pp_plainball,
				.pos = halation_calc_orb_pos(center, rotation, p, projs),
				.color = &clr,
				.rule = halation_orb,
				.args = {
					center, rotation, p + I * projs, halate_time
				},
				.max_viewport_dist = 200,
				.flags = PFLAG_NOCLEAR | PFLAG_NOCOLLISION,
			);
		}
	}

	AT(100 + interval * projs/2) {
		aniplayer_queue(&c->ani,"main",0);

		if(cabs(global.plr.pos-center)>cabs(halation_calc_orb_pos(0,0,0,projs))) {
			char *text[] = {
				"",
				"What are you doing??",
				"Dodge it properly!",
				"I bet you can’t do it! Idiot!",
				"I spent so much time on this attack!",
				"Maybe it is too smart for secondaries!",
				"I think you don’t even understand the timer!",
				"You- You Idiootttt!",
			};

			if(cheater < sizeof(text)/sizeof(text[0])) {
				stagetext_add(text[cheater], global.boss->pos+100*I, ALIGN_CENTER, get_font("standard"), RGB(1,1,1), 0, 100, 10, 20);
				cheater++;
			}
		}
	}
}

static int cirno_icicles(Projectile *p, int t) {
	int turn = 60;

	if(t < 0) {
		if(t == EVENT_BIRTH) {
			p->angle = carg(p->args[0]);
		}

		return ACTION_ACK;
	}

	if(t < turn) {
		p->pos += p->args[0]*pow(0.9,t);
	} else if(t == turn) {
		p->args[0] = 2.5*cexp(I*(carg(p->args[0])-M_PI/2.0+M_PI*(creal(p->args[0]) > 0)));
		if(global.diff > D_Normal)
			p->args[0] += 0.05*nfrand();
		play_sound("redirect");
		spawn_projectile_highlight_effect(p);
	} else if(t > turn) {
		p->pos += p->args[0];
	}

	p->angle = carg(p->args[0]);

	return ACTION_NONE;
}

void cirno_icicle_fall(Boss *c, int time) {
	int t = time % 400;
	TIMER(&t);

	if(time < 0)
		return;

	GO_TO(c, VIEWPORT_W/2.0+120.0*I, 0.01);

	AT(20)
		aniplayer_queue(&c->ani,"(9)",0);
	AT(200)
		aniplayer_queue(&c->ani,"main",0);

	FROM_TO(20,200,30-3*global.diff) {
		play_sound("shot1");
		for(float i = 2-0.2*global.diff; i < 5; i+=1./(1+global.diff)) {
			PROJECTILE(.proto = pp_crystal, .pos = c->pos, .color = RGB(0.3,0.3,0.9), .rule = cirno_icicles, .args = { 6*i*cexp(I*(-0.1+0.1*_i)) });
			PROJECTILE(.proto = pp_crystal, .pos = c->pos, .color = RGB(0.3,0.3,0.9), .rule = cirno_icicles, .args = { 6*i*cexp(I*(M_PI+0.1-0.1*_i)) });
		}
	}

	if(global.diff > D_Easy) {
		FROM_TO_SND("shot1_loop",120,200,3) {
			float f = frand()*_i;

			PROJECTILE(.proto = pp_ball, .pos = c->pos, .color = RGB(0.,0.,0.3), .rule = accelerated, .args = { 0.2*(-2*I-1.5+f),-0.02*I });
			PROJECTILE(.proto = pp_ball, .pos = c->pos, .color = RGB(0.,0.,0.3), .rule = accelerated, .args = { 0.2*(-2*I+1.5+f),-0.02*I });
		}
	}

	if(global.diff > D_Normal) {
		FROM_TO(300,400,10) {
			play_sound("shot1");
			float x = VIEWPORT_W/2+VIEWPORT_W/2*(0.3+_i/10.);
			float angle1 = M_PI/10*frand();
			float angle2 = M_PI/10*frand();
			for(float i = 1; i < 5; i++) {
				PROJECTILE(
					.proto = pp_ball,
					.pos = x,
					.color = RGB(0.,0.,0.3),
					.rule = accelerated,
					.args = {
						i*I*0.5*cexp(I*angle1),
						0.001*I-(global.diff == D_Lunatic)*0.001*frand()
					}
				);

				PROJECTILE(
					.proto = pp_ball,
					.pos = VIEWPORT_W-x,
					.color = RGB(0.,0.,0.3),
					.rule = accelerated,
					.args = {
						i*I*0.5*cexp(-I*angle2),
						0.001*I+(global.diff == D_Lunatic)*0.001*frand()
					}
				);
			}
		}
	}


}

static int cirno_crystal_blizzard_proj(Projectile *p, int time) {
	if(time < 0) {
		return ACTION_ACK;
	}

	if(!(time % 12)) {
		spawn_stain(p->pos, global.frames * 15, 20);
	}

	if(time > 100 + global.diff * 100) {
		p->args[0] *= 1.03;
	}

	return asymptotic(p, time);
}

void cirno_crystal_blizzard(Boss *c, int time) {
	int t = time % 700;
	TIMER(&t);

	if(time < 0) {
		GO_TO(c, VIEWPORT_W/2.0+300*I, 0.1);
		return;
	}

	FROM_TO(60, 360, 10) {
		play_sound("shot1");
		int i, cnt = 14 + global.diff * 3;
		for(i = 0; i < cnt; ++i) {
			PROJECTILE(
				.proto = pp_crystal,
				.pos = i*VIEWPORT_W/cnt,
				.color = i % 2? RGB(0.2,0.2,0.4) : RGB(0.5,0.5,0.5),
				.rule = accelerated,
				.args = {
					0, 0.02*I + 0.01*I * (i % 2? 1 : -1) * sin((i*3+global.frames)/30.0)
				},
			);
		}
	}

	AT(330)
		aniplayer_queue(&c->ani,"(9)",0);
	AT(699)
		aniplayer_queue(&c->ani,"main",0);
	FROM_TO_SND("shot1_loop",330, 700, 1) {
		GO_TO(c, global.plr.pos, 0.01);

		if(!(time % (1 + D_Lunatic - global.diff))) {
			tsrand_fill(2);
			PROJECTILE(
				.proto = pp_wave,
				.pos = c->pos,
				.color = RGBA(0.2, 0.2, 0.4, 0.0),
				.rule = cirno_crystal_blizzard_proj,
				.args = {
					20 * (0.1 + 0.1 * anfrand(0)) * cexp(I*(carg(global.plr.pos - c->pos) + anfrand(1) * 0.2)),
					5
				},
			);
		}

		if(!(time % 7)) {
			play_sound("shot1");
			int i, cnt = global.diff - 1;
			for(i = 0; i < cnt; ++i) {
				PROJECTILE(
					.proto = pp_ball,
					.pos = c->pos,
					.color = RGBA(0.1, 0.1, 0.5, 0.0),
					.rule = accelerated,
					.args = { 0, 0.01 * cexp(I*(global.frames/20.0 + 2*i*M_PI/cnt)) },
				);
			}
		}
	}
}

void cirno_benchmark(Boss* b, int t) {
	if(t < 0) {
		return;
	}

	int N = 5000; // number of particles on the screen

	if(t == 0) {
		aniplayer_queue(&b->ani, "(9)", 0);
	}
	double speed = 10;
	int c = N*speed/VIEWPORT_H;
	for(int i = 0; i < c; i++) {
		double x = frand()*VIEWPORT_W;
		double plrx = creal(global.plr.pos);
		x = plrx + sqrt((x-plrx)*(x-plrx)+100)*(1-2*(x<plrx));

		Projectile *p = PROJECTILE(
			.proto = pp_ball,
			.pos = x,
			.color = RGB(0.1, 0.1, 0.5),
			.rule = linear,
			.args = { speed*I },
			.flags = PFLAG_NOGRAZE,
		);

		if(frand() < 0.1) {
			p->flags &= ~PFLAG_NOGRAZE;
		}

		if(t > 700 && frand() > 0.5)
			projectile_set_prototype(p, pp_plainball);

		if(t > 1200 && frand() > 0.5)
			p->color = *RGB(1.0, 0.2, 0.8);

		if(t > 350 && frand() > 0.5)
			p->color.a = 0;
	}
}

attr_unused
static void cirno_superhardspellcard(Boss *c, int t) {
	// HOWTO: create a super hard spellcard in a few seconds

	cirno_iceplosion0(c, t);
	cirno_iceplosion1(c, t);
	cirno_crystal_rain(c, t);
	cirno_icicle_fall(c, t);
	cirno_icy(c, t);
	cirno_perfect_freeze(c, t);
}

static Boss *create_cirno(void) {
	Boss* cirno = stage1_spawn_cirno(-230 + 100.0*I);

	boss_add_attack(cirno, AT_Move, "Introduction", 2, 0, cirno_intro_boss, NULL);
	boss_add_attack(cirno, AT_Normal, "Iceplosion 0", 20, 23000, cirno_iceplosion0, NULL);
	boss_add_attack_from_info(cirno, &stage1_spells.boss.crystal_rain, false);
	boss_add_attack(cirno, AT_Normal, "Iceplosion 1", 20, 24000, cirno_iceplosion1, NULL);

	if(global.diff > D_Normal) {
		boss_add_attack_from_info(cirno, &stage1_spells.boss.snow_halation, false);
	}

	boss_add_attack_from_info(cirno, &stage1_spells.boss.icicle_fall, false);
	boss_add_attack_from_info(cirno, &stage1_spells.extra.crystal_blizzard, false);

	boss_start_attack(cirno, cirno->attacks);
	return cirno;
}

static int stage1_burst(Enemy *e, int time) {
	TIMER(&time);

	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 1, ITEM_POWER, 1);
		return ACTION_ACK;
	}

	FROM_TO(0, 60, 1) {
		// e->pos += 2.0*I;
		GO_TO(e, e->pos0 + 120*I, 0.03);
	}

	AT(60) {
		int i = 0;
		int n = 1.5*global.diff-1;

		play_sound("shot1");
		for(i = -n; i <= n; i++) {
			PROJECTILE(
				.proto = pp_crystal,
				.pos = e->pos,
				.color = RGB(0.2, 0.3, 0.5),
				.rule = asymptotic,
				.args = {
					(2+0.1*global.diff)*cexp(I*(carg(global.plr.pos - e->pos) + 0.2*i)),
					5
				}
			);
		}

		e->moving = true;
		e->dir = creal(e->args[0]) < 0;

		e->pos0 = e->pos;
	}

	FROM_TO(70, 900, 1) {
		e->pos = e->pos0 + (0.04*e->args[0])*_i*_i;
	}

	return 1;
}

static int stage1_circletoss(Enemy *e, int time) {
	TIMER(&time);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 2, ITEM_POWER, 1);
		return 1;
	}

	e->pos += e->args[0];

	int inter = 2+(global.diff<D_Hard);
	int dur = 40;
	FROM_TO_SND("shot1_loop",60,60+dur,inter) {
		e->args[0] = 0.8*e->args[0];
		PROJECTILE(
			.proto = pp_rice,
			.pos = e->pos,
			.color = RGB(0.6, 0.2, 0.7),
			.rule = asymptotic,
			.args = {
				2*cexp(I*2*M_PI*inter/dur*_i),
				_i/2.0
			}
		);
	}

	if(global.diff > D_Easy) {
		FROM_TO_INT_SND("shot1_loop",90,500,150,5+7*global.diff,1) {
			tsrand_fill(2);
			PROJECTILE(
				.proto = pp_thickrice,
				.pos = e->pos,
				.color = RGB(0.2, 0.4, 0.8),
				.rule = asymptotic,
				.args = {
					(1+afrand(0)*2)*cexp(I*carg(global.plr.pos - e->pos)+0.05*I*global.diff*anfrand(1)),
					3
				}
			);
		}
	}

	FROM_TO(global.diff > D_Easy ? 500 : 240, 900, 1)
		e->args[0] += 0.03*e->args[1] - 0.04*I;

	return 1;
}

static int stage1_sinepass(Enemy *e, int time) {
	TIMER(&time);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 1);
		return 1;
	}

	e->args[1] -= cimag(e->pos-e->pos0)*0.03*I;
	e->pos += e->args[1]*0.4 + e->args[0];

	if(frand() > 0.997-0.005*(global.diff-1)) {
		play_sound("shot1");
		PROJECTILE(
			.proto = pp_ball,
			.pos = e->pos,
			.color = RGB(0.8,0.8,0.4),
			.rule = linear,
			.args = {
				(1+0.2*global.diff+frand())*cexp(I*carg(global.plr.pos - e->pos))
			}
		);
	}

	return 1;
}

static int stage1_drop(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 2);
		return 1;
	}
	if(t < 0)
		return 1;

	e->pos = e->pos0 + e->args[0]*t + e->args[1]*t*t;

	FROM_TO(10,1000,1) {
		if(frand() > 0.997-0.007*(global.diff-1)) {
			play_sound("shot1");
			PROJECTILE(
				.proto = pp_ball,
				.pos = e->pos,
				.color = RGB(0.8,0.8,0.4),
				.rule = linear,
				.args = {
					(1+0.3*global.diff+frand())*cexp(I*carg(global.plr.pos - e->pos))
				}
			);
		}
	}

	return 1;
}

static int stage1_circle(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 3, ITEM_POWER, 2);
		return 1;
	}

	FROM_TO(0, 150, 1)
		e->pos += (e->args[0] - e->pos)*0.02;

	FROM_TO_INT_SND("shot1_loop",150, 550, 40, 40, 2+2*(global.diff<D_Hard)) {
		PROJECTILE(
			.proto = pp_rice,
			.pos = e->pos,
			.color = RGB(0.6, 0.2, 0.7),
			.rule = asymptotic,
			.args = {
				(1.7+0.2*global.diff)*cexp(I*M_PI/10*_ni),
				_ni/2.0
			}
		);
	}

	FROM_TO(560,1000,1)
		e->pos += e->args[1];

	return 1;
}

static int stage1_multiburst(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 3, ITEM_POWER, 2);
		return 1;
	}

	FROM_TO(0, 100, 1) {
		GO_TO(e, e->pos0 + 100*I , 0.02);
	}

	FROM_TO_INT(60, 300, 70, 40, 18-2*global.diff) {
		play_sound("shot1");
		int n = global.diff-1;
		for(int i = -n; i <= n; i++) {
			PROJECTILE(
				.proto = pp_crystal,
				.pos = e->pos,
				.color = RGB(0.2, 0.3, 0.5),
				.rule = linear,
				.args = {
					2.5*cexp(I*(carg(global.plr.pos - e->pos) + i/5.0))
				}
			);
		}
	}

	FROM_TO(320, 700, 1) {
		e->args[1] += 0.03;
		e->pos += e->args[0]*e->args[1] + 1.4*I;
	}

	return 1;
}

static int stage1_instantcircle(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 2, ITEM_POWER, 4);
		return 1;
	}

	AT(75) {
		play_sound("shot_special1");
		for(int i = 0; i < 20+2*global.diff; i++) {
			PROJECTILE(
				.proto = pp_rice,
				.pos = e->pos,
				.color = RGB(0.6, 0.2, 0.7),
				.rule = asymptotic,
				.args = {
					1.5*cexp(I*2*M_PI/(20.0+global.diff)*i),
					2.0,
				},
			);
		}
	}

	AT(95) {
		if(global.diff > D_Easy) {
			play_sound("shot_special1");
			for(int i = 0; i < 20+3*global.diff; i++) {
				PROJECTILE(
					.proto = pp_rice,
					.pos = e->pos,
					.color = RGB(0.6, 0.2, 0.7),
					.rule = asymptotic,
					.args = {
						3*cexp(I*2*M_PI/(20.0+global.diff)*i),
						3.0,
					},
				);
			}
		}
	}

	if(t > 200) {
		e->pos += e->args[1];
	} else {
		GO_TO(e, e->pos0 + e->args[0] * 110 , 0.04);
	}

	return 1;
}

static int stage1_tritoss(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 5, ITEM_POWER, 2);
		return 1;
	}

	FROM_TO(0, 100, 1) {
		e->pos += e->args[0];
	}

	FROM_TO(120, 800,8-global.diff) {
		play_sound("shot1");
		float a = M_PI/30.0*((_i/7)%30)+0.1*nfrand();
		int i;
		int n = 3+global.diff/2;

		for(i = 0; i < n; i++){
			PROJECTILE(
				.proto = pp_thickrice,
				.pos = e->pos,
				.color = RGB(0.2, 0.4, 0.8),
				.rule = asymptotic,
				.args = {
					2*cexp(I*a+2.0*I*M_PI/n*i),
					3,
				},
			);
		}
	}

	FROM_TO(480, 800, 300) {
		play_sound("shot_special1");
		int i, n = 15 + global.diff*3;
		for(i = 0; i < n; i++) {
			PROJECTILE(
				.proto = pp_rice,
				.pos = e->pos,
				.color = RGB(0.6, 0.2, 0.7),
				.rule = asymptotic,
				.args = {
					1.5*cexp(I*2*M_PI/n*i),
					2.0,
				},
			);

			if(global.diff > D_Easy) {
				PROJECTILE(
					.proto = pp_rice,
					.pos = e->pos,
					.color = RGB(0.6, 0.2, 0.7),
					.rule = asymptotic,
					.args = {
						3*cexp(I*2*M_PI/n*i),
						3.0,
					},
				);
			}
		}
	}

	if(t > 820)
		e->pos += e->args[1];

	return 1;
}

// #define BULLET_TEST

#ifdef BULLET_TEST
static int proj_rotate(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	p->angle = global.frames / 60.0;
	// p->angle = M_PI/2;
	return ACTION_NONE;
}
#endif

TASK(burst_fairy, { complex pos; complex dir; }) {
	Enemy *e = TASK_BIND_UNBOXED(create_enemy1c(ARGS.pos, 700, Fairy, NULL, ARGS.dir));

	INVOKE_TASK_WHEN(&e->events.killed, common_drop_items, &e->pos, {
		.points = 1,
		.power = 1,
	});

	e->move.attraction_point = ARGS.pos + 120*I;
	e->move.attraction = 0.03;

	WAIT(60);

	play_sound("shot1");
	int n = 1.5 * global.diff - 1;

	for(int i = -n; i <= n; i++) {
		complex aim = cdir(carg(global.plr.pos - e->pos) + 0.2 * i);

		PROJECTILE(
			.proto = pp_crystal,
			.pos = e->pos,
			.color = RGB(0.2, 0.3, 0.5),
			.move = move_asymptotic_simple(aim * (2 + 0.5 * global.diff), 5),
		);
	}

	WAIT(1);

	e->move.attraction = 0;
	e->move.acceleration = 0.04 * ARGS.dir;
	e->move.retention = 1;

	for(;;) {
		YIELD;
	}
}

TASK(circletoss_shoot_circle, { BoxedEnemy e; int duration; int interval; }) {
	Enemy *e = TASK_BIND(ARGS.e);

	int cnt = ARGS.duration / ARGS.interval;
	double angle_step = M_TAU / cnt;

	for(int i = 0; i < cnt; ++i) {
		play_loop("shot1_loop");
		e->move.velocity *= 0.8;

		complex aim = cdir(angle_step * i);

		PROJECTILE(
			.proto = pp_rice,
			.pos = e->pos,
			.color = RGB(0.6, 0.2, 0.7),
			.move = move_asymptotic_simple(2 * aim, i * 0.5),
		);

		WAIT(ARGS.interval);
	}
}

TASK(circletoss_shoot_toss, { BoxedEnemy e; int times; int duration; int period; }) {
	Enemy *e = TASK_BIND(ARGS.e);

	while(ARGS.times--) {
		for(int i = ARGS.duration; i--;) {
			play_loop("shot1_loop");

			double aim_angle = carg(global.plr.pos - e->pos);
			aim_angle += 0.05 * global.diff * nfrand();

			complex aim = cdir(aim_angle);
			aim *= rand_range(1, 3);

			PROJECTILE(
				.proto = pp_thickrice,
				.pos = e->pos,
				.color = RGB(0.2, 0.4, 0.8),
				.move = move_asymptotic_simple(aim, 3),
			);

			WAIT(1);
		}

		WAIT(ARGS.period - ARGS.duration);
	}
}

TASK(circletoss_fairy, { complex pos; complex velocity; complex exit_accel; int exit_time; }) {
	Enemy *e = TASK_BIND_UNBOXED(create_enemy1c(ARGS.pos, 1500, BigFairy, NULL, 0));

	e->move = move_linear(ARGS.velocity);

	INVOKE_TASK_WHEN(&e->events.killed, common_drop_items, &e->pos, {
		.points = 2,
		.power = 1,
	});

	INVOKE_TASK_DELAYED(60, circletoss_shoot_circle, ENT_BOX(e),
		.duration = 40,
		.interval = 2 + (global.diff < D_Hard)
	);

	if(global.diff > D_Easy) {
		INVOKE_TASK_DELAYED(90, circletoss_shoot_toss, ENT_BOX(e),
			.times = 4,
			.period = 150,
			.duration = 5 + 7 * global.diff
		);
	}

	WAIT(ARGS.exit_time);
	e->move.acceleration += ARGS.exit_accel;
	STALL;
}

TASK(sinepass_swirl_move, { BoxedEnemy e; complex v; complex sv; }) {
	Enemy *e = TASK_BIND(ARGS.e);
	complex sv = ARGS.sv;
	complex v = ARGS.v;

	for(;;) {
		sv -= cimag(e->pos - e->pos0) * 0.03 * I;
		e->pos += sv * 0.4 + v;
		YIELD;
	}
}

TASK(sinepass_swirl, { complex pos; complex vel; complex svel; }) {
	Enemy *e = TASK_BIND_UNBOXED(create_enemy1c(ARGS.pos, 100, Swirl, NULL, 0));

	INVOKE_TASK_WHEN(&e->events.killed, common_drop_items, &e->pos, {
		.points = 1,
	});

	INVOKE_TASK(sinepass_swirl_move, ENT_BOX(e), ARGS.vel, ARGS.svel);

	WAIT(difficulty_value(25, 20, 15, 10));

	int shot_interval = difficulty_value(120, 40, 30, 20);

	for(;;) {
		play_sound("shot1");

		complex aim = cnormalize(global.plr.pos - e->pos);
		aim *= difficulty_value(2, 2, 2.5, 3);

		PROJECTILE(
			.proto = pp_ball,
			.pos = e->pos,
			.color = RGB(0.8, 0.8, 0.4),
			.move = move_asymptotic_simple(aim, 5),
		);

		WAIT(shot_interval);
	}
}

TASK(circle_fairy, { complex pos; complex target_pos; }) {
	Enemy *e = TASK_BIND_UNBOXED(create_enemy1c(ARGS.pos, 1400, BigFairy, NULL, 0));

	INVOKE_TASK_WHEN(&e->events.killed, common_drop_items, &e->pos, {
		.points = 3,
		.power = 2,
	});

	e->move.attraction = 0.005;
	e->move.retention = 0.8;
	e->move.attraction_point = ARGS.target_pos;

	WAIT(120);

	int shot_interval = 2;
	int shot_count = difficulty_value(10, 10, 20, 25);
	int round_interval = 120 - shot_interval * shot_count;

	for(int round = 0; round < 2; ++round) {
		double a_ofs = rand_angle();

		for(int i = 0; i < shot_count; ++i) {
			complex aim;

			aim = circle_dir_ofs((round & 1) ? i : shot_count - i, shot_count, a_ofs);
			aim *= difficulty_value(1.7, 2.0, 2.5, 2.5);

			PROJECTILE(
				.proto = pp_rice,
				.pos = e->pos,
				.color = RGB(0.6, 0.2, 0.7),
				.move = move_asymptotic_simple(aim, i * 0.5),
			);

			play_loop("shot1_loop");
			WAIT(shot_interval);
		}

		e->move.attraction_point += 30 * cdir(rand_angle());
		WAIT(round_interval);
	}

	WAIT(10);
	e->move.attraction = 0;
	e->move.retention = 1;
	e->move.acceleration = -0.04 * I * cdir(nfrand() * M_TAU / 12);
	STALL;
}

TASK(drop_swirl, { complex pos; complex vel; complex accel; }) {
	Enemy *e = TASK_BIND_UNBOXED(create_enemy1c(ARGS.pos, 100, Swirl, NULL, 0));

	INVOKE_TASK_WHEN(&e->events.killed, common_drop_items, &e->pos, {
		.points = 2,
	});

	e->move = move_accelerated(ARGS.vel, ARGS.accel);

	int shot_interval = difficulty_value(120, 40, 30, 20);

	WAIT(20);

	while(true) {
		complex aim = cnormalize(global.plr.pos - e->pos);
		aim *= 1 + 0.3 * global.diff + frand();

		play_sound("shot1");
		PROJECTILE(
			.proto = pp_ball,
			.pos = e->pos,
			.color = RGB(0.8, 0.8, 0.4),
			.move = move_linear(aim),
		);

		WAIT(shot_interval);
	}
}

TASK(multiburst_fairy, { complex pos; complex target_pos; complex exit_accel; }) {
	Enemy *e = TASK_BIND_UNBOXED(create_enemy1c(ARGS.pos, 1000, Fairy, NULL, 0));

	INVOKE_TASK_WHEN(&e->events.killed, common_drop_items, &e->pos, {
		.points = 3,
		.power = 2,
	});

	e->move.attraction = 0.05;
	// e->move.retention = 0.8;
	e->move.attraction_point = ARGS.target_pos;

	WAIT(60);

	int burst_interval = difficulty_value(22, 20, 18, 16);
	int bursts = 4;

	for(int i = 0; i < bursts; ++i) {
		play_sound("shot1");
		int n = global.diff - 1;

		for(int j = -n; j <= n; j++) {
			complex aim = cdir(carg(global.plr.pos - e->pos) + j / 5.0);
			aim *= 2.5;

			PROJECTILE(
				.proto = pp_crystal,
				.pos = e->pos,
				.color = RGB(0.2, 0.3, 0.5),
				.move = move_linear(aim),
			);
		}

		WAIT(burst_interval);
	}

	WAIT(10);
	e->move.attraction = 0;
	e->move.retention = 1;
	e->move.acceleration = ARGS.exit_accel;
	STALL;
}

// opening. projectile bursts
TASK(burst_fairies_1, NO_ARGS) {
	for(int i = 3; i--;) {
		INVOKE_TASK(burst_fairy, VIEWPORT_W/2 + 70,  1 + 0.6*I);
		INVOKE_TASK(burst_fairy, VIEWPORT_W/2 - 70, -1 + 0.6*I);
		stage_wait(25);
	}
}

// more bursts. fairies move / \ like
TASK(burst_fairies_2, NO_ARGS) {
	for(int i = 3; i--;) {
		double ofs = 70 + i * 40;
		INVOKE_TASK(burst_fairy, ofs,               1 + 0.6*I);
		stage_wait(15);
		INVOKE_TASK(burst_fairy, VIEWPORT_W - ofs, -1 + 0.6*I);
		stage_wait(15);
	}
}

TASK(burst_fairies_3, NO_ARGS) {
	for(int i = 10; i--;) {
		complex pos = VIEWPORT_W/2 - 200 * sin(1.17 * global.frames);
		INVOKE_TASK(burst_fairy, pos, sign(nfrand()));
		stage_wait(60);
	}
}

// swirl, sine pass
TASK(sinepass_swirls, { int duration; double level; double dir; }) {
	int duration = ARGS.duration;
	double dir = ARGS.dir;
	complex pos = CMPLX(ARGS.dir < 0 ? VIEWPORT_W : 0, ARGS.level);
	int delay = difficulty_value(30, 20, 15, 10);

	for(int t = 0; t < duration; t += delay) {
		INVOKE_TASK(sinepass_swirl, pos, 3.5 * dir, 7.0 * I);
		stage_wait(delay);
	}
}

// big fairies, circle + projectile toss
TASK(circletoss_fairies_1, NO_ARGS) {
	for(int i = 0; i < 2; ++i) {
		INVOKE_TASK(circletoss_fairy,
			.pos = VIEWPORT_W * i + VIEWPORT_H / 3 * I,
			.velocity = 2 - 4 * i - 0.3 * I,
			.exit_accel = 0.03 * (1 - 2 * i) - 0.04 * I ,
			.exit_time = (global.diff > D_Easy) ? 500 : 240
		);

		stage_wait(50);
	}
}

TASK(drop_swirls, { int cnt; complex pos; complex vel; complex accel; }) {
	for(int i = 0; i < ARGS.cnt; ++i) {
		INVOKE_TASK(drop_swirl, ARGS.pos, ARGS.vel, ARGS.accel);
		stage_wait(20);
	}
}

TASK(schedule_swirls, NO_ARGS) {
	INVOKE_TASK(drop_swirls, 25, VIEWPORT_W/3, 2*I, 0.06);
	stage_wait(400);
	INVOKE_TASK(drop_swirls, 25, 200*I, 4, -0.06*I);
}

TASK(circle_fairies_1, NO_ARGS) {
	for(int i = 0; i < 3; ++i) {
		for(int j = 0; j < 3; ++j) {
			INVOKE_TASK(circle_fairy, VIEWPORT_W - 64, VIEWPORT_W/2 - 100 + 200 * I + 128 * j);
			stage_wait(60);
		}

		stage_wait(90);

		for(int j = 0; j < 3; ++j) {
			INVOKE_TASK(circle_fairy, 64, VIEWPORT_W/2 + 100 + 200 * I - 128 * j);
			stage_wait(60);
		}

		stage_wait(240);
	}
}

TASK(multiburst_fairies_1, NO_ARGS) {
	for(int row = 0; row < 3; ++row) {
		for(int col = 0; col < 5; ++col) {
			log_debug("WTF %i %i", row, col);
			complex pos = VIEWPORT_W * frand();
			complex target_pos = 64 + 64 * col + I * (64 * row + 100);
			complex exit_accel = 0.02 * I + 0.03;
			INVOKE_TASK(multiburst_fairy, pos, target_pos, exit_accel);

			WAIT(10);
		}

		WAIT(120);
	}
}

TASK_WITH_INTERFACE(midboss_intro, BossAttack) {
	Boss *boss = TASK_BIND(ARGS.boss);
	WAIT_EVENT(&ARGS.attack->events.started);
	boss->move = move_towards(VIEWPORT_W/2.0 + 100.0*I, 0.035);
}



TASK(cirno_snowflake_proj_twist, { BoxedProjectile p; }) {
	Projectile *p = ENT_UNBOX(ARGS.p);

	if(p != NULL) {
		play_sound_ex("redirect", 30, false);
		play_sound_ex("shot_special1", 30, false);
		color_lerp(&p->color, RGB(0.5, 0.5, 0.5), 0.5);
		spawn_projectile_highlight_effect(p);
		p->move.velocity = -cabs(p->move.velocity)*cdir(p->angle);
	}
}

TASK_WITH_INTERFACE(icy_storm, BossAttack) {
	Boss *boss = TASK_BIND(ARGS.boss);
	Attack *a = ARGS.attack;

	CANCEL_TASK_WHEN(&a->events.finished, THIS_TASK);
	WAIT_EVENT(&a->events.started);
	
	int interval = 70 - 8 * global.diff;

	for(int run = 0;; run++) {
		complex vel = (1+0.125*global.diff)*cexp(I*fmod(200*run,M_PI));
		int c = 6;
		double dr = 15;
		int size = 5+3*sin(337*run);

		int start_time = global.frames;
		
		for(int j = 0; j < size; j++) {
			WAIT(3);
			play_loop("shot1_loop");
			for(int i = 0; i < c; i++) {
				double ang = 2*M_PI/c*i+run*515;
				complex phase = cdir(ang);
				int t = global.frames-start_time;
				complex pos = boss->pos+vel*t+dr*j*phase;

				int split_time = 200 - 20*global.diff - j*3;
				Projectile *p;

				for(int side = -1; side <= 1; side += 2) {
					p = PROJECTILE(
						.proto = pp_crystal,
						.pos = pos+side*6*I*phase,
						.color = RGB(0.0, 0.1 + 0.1 * size / 5, 0.8),
						.move = move_linear(vel),
						.angle = ang+side*M_PI/4,
						.max_viewport_dist = 64,
						.flags = PFLAG_MANUALANGLE,
					);
					INVOKE_TASK_DELAYED(split_time, cirno_snowflake_proj_twist, ENT_BOX(p));
				}

				int split = 3;

				if(j > split) {
					complex pos0 = boss->pos+vel*t+dr*split*phase;

					for(int side = -1; side <= 1; side+=2) {
						complex phase2 = cdir(M_PI/4*side)*phase;
						complex pos2 = pos0+(dr*(j-split))*phase2;

						PROJECTILE(
							.proto = pp_crystal,
							.pos = pos2,
							.color = RGB(0.0,0.3*size/5,1),
							.move = move_linear(vel),
							.angle = ang+M_PI/4*side,
							.max_viewport_dist = 64,
							.flags = PFLAG_MANUALANGLE,
						);
					}
				}
			}

		}

		WAIT(interval-3*size);
	}
	
}

DEFINE_EXTERN_TASK(stage1_spell_perfect_freeze) {
	Boss *boss = TASK_BIND(ARGS.boss);
	Attack *a = ARGS.attack;

	CANCEL_TASK_WHEN(&a->events.finished, THIS_TASK);
	boss->move = move_towards(VIEWPORT_W/2.0 + 100.0*I, 0.04);
	WAIT_EVENT(&a->events.started);

	// TODO implement

	boss->move.retention = 0.8;
	boss->move.attraction = 0.01;

	Rect move_bounds;
	move_bounds.top_left = CMPLX(64, 64);
	move_bounds.bottom_right = CMPLX(VIEWPORT_W - 64, 200);

	for(;;) {
		boss->move.attraction_point = common_wander(boss->pos, rand_range(50, 200), move_bounds);
		WAIT(60);
	}
}

TASK_WITH_INTERFACE(midboss_flee, BossAttack) {
	Boss *boss = TASK_BIND(ARGS.boss);
	WAIT_EVENT(&ARGS.attack->events.started);
	boss->move = move_towards(-250 + 30 * I, 0.02);
}

TASK(spawn_midboss, NO_ARGS) {
	Boss *boss = global.boss = stage1_spawn_cirno(VIEWPORT_W + 220 + 30.0*I);

	boss_add_attack_task(boss, AT_Move, "Introduction", 2, 0, TASK_INDIRECT(BossAttack, midboss_intro), NULL);
	boss_add_attack_task(boss, AT_Normal, "Icy Storm", 20, 24000, TASK_INDIRECT(BossAttack, icy_storm), NULL);
	boss_add_attack_from_info(boss, &stage1_spells.mid.perfect_freeze, false);
	boss_add_attack_task(boss, AT_Move, "Introduction", 2, 0, TASK_INDIRECT(BossAttack, midboss_flee), NULL);

	boss_start_attack(boss, boss->attacks);
}

TASK(stage_timeline, NO_ARGS) {
	stage_start_bgm("stage1");
	stage_set_voltage_thresholds(50, 125, 300, 600);

	/*
	for(;;) {
		complex pos, tpos;

		if(frand() > 0.5) {
			pos = VIEWPORT_W - 64;
		} else {
			pos = 64;
		}

		tpos = rand_range(64, VIEWPORT_W - 64) + 200 * I;

		INVOKE_TASK(circle_fairy, pos, tpos);
		WAIT(240);
	}
	return;
	*/

	INVOKE_TASK_DELAYED(100, burst_fairies_1);
	INVOKE_TASK_DELAYED(240, burst_fairies_2);
	INVOKE_TASK_DELAYED(440, sinepass_swirls, 180, 100, 1);
	INVOKE_TASK_DELAYED(480, circletoss_fairies_1);
	INVOKE_TASK_DELAYED(660, circle_fairies_1);
	INVOKE_TASK_DELAYED(900, schedule_swirls);
	INVOKE_TASK_DELAYED(1500, burst_fairies_3);
	INVOKE_TASK_DELAYED(2200, multiburst_fairies_1);
	INVOKE_TASK_DELAYED(2700, spawn_midboss);
}

void stage1_events(void) {
	TIMER(&global.timer);

	AT(0) {
		INVOKE_TASK(stage_timeline);
	}

	return;

#ifdef BULLET_TEST
	if(!global.projs) {
		PROJECTILE(
			.proto = pp_rice,
			.pos = (VIEWPORT_W + VIEWPORT_H * I) * 0.5,
			.color = hsl(0.5, 1.0, 0.5),
			.rule = proj_rotate,
		);
	}

	if(!(global.frames % 36)) {
		ProjPrototype *projs[] = {
			pp_thickrice,
			pp_rice,
			// pp_ball,
			// pp_plainball,
			// pp_bigball,
			// pp_soul,
			pp_wave,
			pp_card,
			pp_bigball,
			pp_plainball,
			pp_ball,
		};
		int numprojs = sizeof(projs)/sizeof(*projs);

		for(int i = 0; i < numprojs; ++i) {
			PROJECTILE(
				.proto = projs[i],
				.pos = ((0.5 + i) * VIEWPORT_W/numprojs + 0 * I),
				.color = hsl(0.5, 1.0, 0.5),
				.rule = linear,
				.args = { 1*I },
			);
		}

	}

	return;
#endif

	// opening. projectile bursts
	FROM_TO(100, 160, 25) {
		create_enemy1c(VIEWPORT_W/2 + 70, 700, Fairy, stage1_burst, 1 + 0.6*I);
		create_enemy1c(VIEWPORT_W/2 - 70, 700, Fairy, stage1_burst, -1 + 0.6*I);
	}

	// more bursts. fairies move / \ like
	FROM_TO(240, 300, 30) {
		create_enemy1c(70 + _i*40, 700, Fairy, stage1_burst, -1 + 0.6*I);
		create_enemy1c(VIEWPORT_W - (70 + _i*40), 700, Fairy, stage1_burst, 1 + 0.6*I);
	}

	// big fairies, circle + projectile toss
	FROM_TO(400, 460, 50)
		create_enemy2c(VIEWPORT_W*_i + VIEWPORT_H/3*I, 1500, BigFairy, stage1_circletoss, 2-4*_i-0.3*I, 1-2*_i);

	// swirl, sine pass
	FROM_TO(380, 1000, 20) {
		tsrand_fill(2);
		create_enemy2c(VIEWPORT_W*(_i&1) + afrand(0)*100.0*I + 70.0*I, 100, Swirl, stage1_sinepass, 3.5*(1-2*(_i&1)), afrand(1)*7.0*I);
	}

	// swirl, drops
	FROM_TO(1100, 1600, 20)
		create_enemy2c(VIEWPORT_W/3, 100, Swirl, stage1_drop, 4.0*I, 0.06);

	FROM_TO(1500, 2000, 20)
		create_enemy2c(VIEWPORT_W+200.0*I, 100, Swirl, stage1_drop, -2, -0.04-0.03*I);

	// bursts
	FROM_TO(1250, 1800, 60) {
		create_enemy1c(VIEWPORT_W/2 - 200 * sin(1.17*global.frames), 500, Fairy, stage1_burst, nfrand());
	}

	// circle - multi burst combo
	FROM_TO(1700, 2300, 300) {
		tsrand_fill(3);
		create_enemy2c(VIEWPORT_W/2, 1400, BigFairy, stage1_circle, VIEWPORT_W/4 + VIEWPORT_W/2*afrand(0)+200.0*I, 3-6*(afrand(1)>0.5)+afrand(2)*2.0*I);
	}

	FROM_TO(2000, 2500, 200) {
		int t = global.diff + 1;
		for(int i = 0; i < t; i++)
			create_enemy1c(VIEWPORT_W/2 - 40*t + 80*i, 1000, Fairy, stage1_multiburst, i - 2.5);
	}

	AT(2700)
		global.boss = create_cirno_mid();

	// some chaotic swirls + instant circle combo
	FROM_TO(2760, 3800, 20) {
		tsrand_fill(2);
		create_enemy2c(VIEWPORT_W/2 - 200*anfrand(0), 250+40*global.diff, Swirl, stage1_drop, 1.0*I, 0.001*I + 0.02 + 0.06*anfrand(1));
	}

	FROM_TO(2900, 3750, 190-30*global.diff) {
		create_enemy2c(VIEWPORT_W/2 + 205 * sin(2.13*global.frames), 1200, Fairy, stage1_instantcircle, 2.0*I, 3.0 - 6*frand() - 1.0*I);
	}

	// multiburst + normal circletoss, later tri-toss
	FROM_TO(3900, 4800, 200) {
		create_enemy1c(VIEWPORT_W/2 - 195 * cos(2.43*global.frames), 1000, Fairy, stage1_multiburst, 2.5*frand());
	}

	FROM_TO(4000, 4100, 20)
		create_enemy2c(VIEWPORT_W*_i + VIEWPORT_H/3*I, 1700, Fairy, stage1_circletoss, 2-4*_i-0.3*I, 1-2*_i);

	AT(4200)
		create_enemy2c(VIEWPORT_W/2.0, 4000, BigFairy, stage1_tritoss, 2.0*I, -2.6*I);

	AT(5000) {
		enemy_kill_all(&global.enemies);
		stage_unlock_bgm("stage1");
		global.boss = create_cirno();
	}

	AT(5100) {
		stage_unlock_bgm("stage1boss");
		global.dialog = stage1_dialog_post_boss();
	}

	AT(5105) {
		stage_finish(GAMEOVER_SCORESCREEN);
	}
}
