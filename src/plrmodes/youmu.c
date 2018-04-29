/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "global.h"
#include "plrmodes.h"
#include "youmu.h"

PlayerCharacter character_youmu = {
	.id = PLR_CHAR_YOUMU,
	.lower_name = "youmu",
	.proper_name = "Yōmu",
	.full_name = "Konpaku Yōmu",
	.title = "Half-Phantom Girl",
	.dialog_sprite_name = "dialog/youmu",
	.player_sprite_name = "player/youmu",
	.ending = {
		.good = good_ending_youmu,
		.bad = bad_ending_youmu,
	},
};

void youmu_common_shot(Player *plr) {
	if(!(global.frames % 4)) {
		play_sound("generic_shot");
	}

	if(!(global.frames % 6)) {
		Color c = rgb(1, 1, 1);

		PROJECTILE(
			.proto = pp_youmu,
			.pos = plr->pos + 10 - I*20,
			.color = c,
			.rule = linear,
			.args = { -20.0*I },
			.type = PlrProj+120,
			.shader = "sprite_default",
		);

		PROJECTILE(
			.proto = pp_youmu,
			.pos = plr->pos - 10 - I*20,
			.color = c,
			.rule = linear,
			.args = { -20.0*I },
			.type = PlrProj+120,
			.shader = "sprite_default",
		);
	}
}

void youmu_common_bombbg(Player *plr) {
	float t = player_get_bomb_progress(&global.plr, NULL);
	float fade = 1;

	if(t < 1./12)
		fade = t*12;

	if(t > 1./2)
		fade = 1-(t-1./2)*2;

	if(fade < 0)
		fade = 0;

	r_color4(1,1,1,0.6*fade);
	fill_viewport_p(0.5,0.5,3,1,1200*t*(t-1.5),get_tex("youmu_bombbg1"));
	r_color4(1,1,1,1);
}

void youmu_common_draw_proj(Projectile *p, Color c, float scale) {
	r_mat_push();
	r_mat_translate(creal(p->pos), cimag(p->pos), 0);
	r_mat_rotate_deg(p->angle*180/M_PI+90, 0, 0, 1);
	r_mat_scale(scale, scale, 1);
	ProjDrawCore(p, c);
	r_mat_pop();
}

