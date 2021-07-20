/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "../stagex.h"
#include "global.h"
#include "common_tasks.h"

static void spawn_circle(cmplx pos, real phase, real radius, int count, real collapse_time) {
	for(int i = 0; i < count; i++) {
		cmplx offset = radius*cdir(M_TAU/count*(i+phase));

		PROJECTILE(
			.pos = pos + offset,
			.proto = pp_bullet,
			.color = RGBA(0.5,0.2,1,1),
			.move = move_linear(-offset/collapse_time),
			.timeout = collapse_time,
			.max_viewport_dist = 2*radius,
		);
	}
}

static void draw_wriggle_proj(Projectile *p, int t, ProjDrawRuleArgs args) {
	Animation *ani = res_anim("boss/wriggle");
	AniSequence *seq = get_ani_sequence(ani, "fly");
 	r_draw_sprite(&(SpriteParams){
	 	.shader = "sprite_default",
	 	.pos.as_cmplx = p->pos,
	 	.scale.as_cmplx = p->scale,
	 	.sprite_ptr = animation_get_frame(ani, seq, global.frames),
	 	.color = &p->color,
	 	.rotation.angle = p->angle+M_PI/2,
 	});
}

DEFINE_EXTERN_TASK(stagex_spell_trap_representation) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);
	boss->move = move_towards(CMPLX(VIEWPORT_W/2, VIEWPORT_H/2), 0.02);

	real radius = 200;
	int count = 20;
	int final_count = 15;
	WAIT(60);

	for(;;) {
		for(int n = 2; n <= 4; n++) {
			int c = 1<<(n-1);
			if(n == 4) {
				INVOKE_SUBTASK(common_charge, {boss->pos, RGBA(0.5,0.6,2.0,0.0), 120, .sound = COMMON_CHARGE_SOUNDS});
			}
			for(int i = 0; i < c; i++) {
				play_sfx("shot1");
				spawn_circle(global.plr.pos, 0, radius, count, 4*120/n);
				WAIT(120/c);
			}
		}
		for(int i = 0; i < final_count; i++) {
			cmplx vel = cnormalize(global.plr.pos-boss->pos)*cdir(M_TAU/final_count*i);
			PROJECTILE(
				.pos = boss->pos,
				.draw_rule = {draw_wriggle_proj},
				.size = CMPLX(75,100),
				.collision_size = CMPLX(60,70),
				.color = RGBA(1.0,1.0,1,0.5),
				.move = move_accelerated(vel,0.05*vel),
			);
		}
		boss->pos = CMPLX(VIEWPORT_W*rng_real(), -300);

		WAIT(60);
	}
}
