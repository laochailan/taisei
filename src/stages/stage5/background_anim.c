/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "background_anim.h"
#include "draw.h"

#include "global.h"
#include "stageutils.h"
#include "common_tasks.h"

TASK(stage5_bg_update, NO_ARGS) {
	Stage5DrawData *stage5_draw_data = stage5_get_draw_data();

	Camera3D *cam = &stage_3d_context.cam;
	float vel = -0.015/3.7;
	for(int i = 0;; i++) {
		float r = stage5_draw_data->stairs.rad;
		stage3d_update(&stage_3d_context);
		cam->rot.v[2] = 180/M_PI*vel*i + stage5_draw_data->stairs.roffset;
		cam->pos[0] = r*cos(vel*i);
		cam->pos[1] = r*sin(vel*i);

		cam->pos[2] = stage5_draw_data->stairs.zoffset - 11.2/M_TAU*vel*i;

		stage5_draw_data->stairs.light_strength *= 0.975;

		if(rng_chance(0.007)) {
			stage5_draw_data->stairs.light_strength = fmax(stage5_draw_data->stairs.light_strength, rng_range(5, 10));
		}

		YIELD;
	}
}

void stage5_bg_init_fullstage(void) {
	Camera3D *cam = &stage_3d_context.cam;
	cam->pos[0] = 0;
	cam->pos[1] = 2.5;
	cam->rot.v[0] = 80;
	cam->fovy = STAGE3D_DEFAULT_FOVY*1.5;
	INVOKE_TASK(stage5_bg_update);
}

void stage5_bg_init_spellpractice(void) {
	Camera3D *cam = &stage_3d_context.cam;
	cam->pos[0] = 0;
	cam->pos[1] = 2.5;
	cam->rot.v[0] = 45;
	cam->fovy = STAGE3D_DEFAULT_FOVY*1.5;

	Stage5DrawData *stage5_draw_data = stage5_get_draw_data();
	stage5_draw_data->stairs.roffset = -90;
	stage5_draw_data->stairs.zoffset = 5;
	stage5_draw_data->stairs.rad = -3;

	INVOKE_TASK(stage5_bg_update);
}

void stage5_bg_raise_lightning(void) {
	Stage5DrawData *stage5_draw_data = stage5_get_draw_data();
	INVOKE_TASK(common_easing_animate,
		    .value = &stage5_draw_data->stairs.light_pos,
		    .to = 120,
		    .duration = 200,
		    .ease = glm_ease_quad_out
	);
	INVOKE_TASK(common_easing_animate,
		    .value = &stage5_draw_data->stairs.light_strength,
		    .to = 5,
		    .duration = 200,
		    .ease = glm_ease_quad_inout
	);
}

void stage5_bg_lower_camera(void) {
	Stage5DrawData *stage5_draw_data = stage5_get_draw_data();
	INVOKE_TASK(common_easing_animate,
		    .value = &stage5_draw_data->stairs.zoffset,
		    .to = 5,
		    .duration = 600,
		    .ease = glm_ease_quad_inout
	);
	INVOKE_TASK_DELAYED(60,common_easing_animate,
		    .value = &stage5_draw_data->stairs.rad,
		    .to = -3,
		    .duration = 600,
		    .ease = glm_ease_quad_inout
	);
	INVOKE_TASK_DELAYED(60, common_easing_animate,
		    .value = &stage_3d_context.cam.rot.v[0],
		    .to = 45,
		    .duration = 700,
		    .ease = glm_ease_quad_inout
	);
	INVOKE_TASK_DELAYED(120, common_easing_animate,
		    .value = &stage5_draw_data->stairs.roffset,
		    .to = -90,
		    .duration = 400,
		    .ease = glm_ease_quad_inout
	);
}

