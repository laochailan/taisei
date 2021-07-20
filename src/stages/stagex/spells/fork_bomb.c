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

static void draw_scuttle_proj(Projectile *p, int t, ProjDrawRuleArgs args) {
	Animation *ani = res_anim("boss/scuttle");
	AniSequence *seq = get_ani_sequence(ani, "main");
 	r_draw_sprite(&(SpriteParams){
	 	.shader = "sprite_default",
	 	.pos.as_cmplx = p->pos,
	 	.scale.as_cmplx = p->scale,
	 	.sprite_ptr = animation_get_frame(ani, seq, global.frames),
	 	.color = &p->color,
	 	.rotation.angle = p->angle+M_PI/2,
 	});
}

attr_unused // TODO: remove me
TASK(scuttle_proj_death, { cmplx pos; real angle; }) {
	int count = 10;
	for(int i = 0; i < count; i++) {
		real phi = M_TAU/count*i;
		cmplx vel = (2+cos(2*(phi-ARGS.angle)))*cdir(phi);
		PROJECTILE(
			.pos = ARGS.pos,
			.proto = pp_bullet,
			.color = RGBA(1,0.2,0.5,1),
			.move = move_linear(vel),
		);
	}
}

#define FORK_GRID_SIZE 7

TASK(fork_proj, { cmplx pos; cmplx vel; int split_time; int *fork_grid; cmplx direction; int gx; int gy; }) {
	Projectile *p = TASK_BIND(PROJECTILE(
		.pos = ARGS.pos,
		.proto = pp_bigball,
		.color = RGBA(0.5,0.2,1,1),
		.move = move_linear(ARGS.vel),
	));

	WAIT(ARGS.split_time);
	p->move = move_stop(0.01);
	/*int count = 6;
	for(int i = 0; i < count; i++) {
		cmplx vel = 1 * cdir(M_TAU / count * i);
		PROJECTILE(
			.pos = p->pos,
			.proto = pp_bullet,
			.color = RGBA(0.5,0.2,1,1),
			.move = move_linear(vel),
		);
	}*/


	//WAIT(ARGS.split_time);

	if(!ARGS.fork_grid[ARGS.gy*FORK_GRID_SIZE+ARGS.gx]) {
		ARGS.fork_grid[ARGS.gy*FORK_GRID_SIZE+ARGS.gx] = 1;


		cmplx nvel = I*ARGS.vel;
		cmplx ndirection = I*ARGS.direction;

		int ngx1 = ARGS.gx + creal(ndirection);
		int ngy1 = ARGS.gy + cimag(ndirection);
		play_sfx("shot1");
		if(ngx1 >= 0 && ngx1 < FORK_GRID_SIZE && ngy1 >= 0 && ngy1 < FORK_GRID_SIZE && !ARGS.fork_grid[ngy1*FORK_GRID_SIZE+ngx1]) {
			INVOKE_TASK(fork_proj, {p->pos, nvel, ARGS.split_time,
						   .fork_grid = ARGS.fork_grid,
						   .direction = ndirection,
						   .gx = ngx1,
						   .gy = ngy1
			});
		}
		int ngx2 = ARGS.gx - creal(ndirection);
		int ngy2 = ARGS.gy - cimag(ndirection);
		if(ngx2 >= 0 && ngx2 < FORK_GRID_SIZE && ngy2 >= 0 && ngy2 < FORK_GRID_SIZE && !ARGS.fork_grid[ngy2*FORK_GRID_SIZE+ngx2]) {
			INVOKE_TASK(fork_proj, {p->pos, -nvel, ARGS.split_time,
						   .fork_grid = ARGS.fork_grid,
						   .direction = -ndirection,
						   .gx = ngx2,
						   .gy = ngy2
			});
		}

	}
	WAIT(4*ARGS.split_time);

	play_sfx("shot2");
	cmplx aim = cnormalize(global.plr.pos-p->pos);
	Rect shoot_rect = { 0, CMPLX(VIEWPORT_W, VIEWPORT_H) };

	if(point_in_rect(p->pos, shoot_rect)) {
		PROJECTILE(
			.pos = p->pos,
			.draw_rule = {draw_scuttle_proj},
			.size = CMPLX(75, 100),
			.collision_size = 0.5*CMPLX(32, 50),
			.color = RGBA(1, 1, 1, 0.5),
			.angle = carg(ARGS.vel),
			.scale = 0.5,
			.move = move_accelerated(aim,0)
		);
	}
	kill_projectile(p);
}


DEFINE_EXTERN_TASK(stagex_spell_fork_bomb) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_towards(CMPLX(VIEWPORT_W/2, VIEWPORT_H/2), 0.02);
	BEGIN_BOSS_ATTACK(&ARGS);

	int split_time = 30;

	int fork_grid[FORK_GRID_SIZE*FORK_GRID_SIZE];

	real spacing = 100;


	for(int i = 0;; i++) {
		memset(fork_grid, 0, sizeof(fork_grid));

		int gx = FORK_GRID_SIZE/2;
		int gy = FORK_GRID_SIZE/2;

		fork_grid[gy*FORK_GRID_SIZE+gx] = 1;

		cmplx vel = spacing/split_time*cdir(M_PI/4*i);
		play_sfx("shot_special1");
		INVOKE_SUBTASK(fork_proj, {boss->pos, vel, split_time,
					   .fork_grid = fork_grid,
					   .direction = 1,
					   .gx = gx+1,
					   .gy = gy
		});
		INVOKE_SUBTASK(fork_proj, {boss->pos, -vel, split_time,
					   .fork_grid = fork_grid,
					   .direction = -1,
					   .gx = gx-1,
					   .gy = gy
		});
		WAIT(5*split_time);
		INVOKE_SUBTASK(common_charge, {boss->pos, RGBA(0.5, 0.6, 2.0, 0.0), 3*split_time, .sound = COMMON_CHARGE_SOUNDS});
		WAIT(3*split_time);
	}
}
