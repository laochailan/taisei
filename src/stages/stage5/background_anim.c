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

TASK(camera_down, { Stage5DrawData *draw_data; }) {
	Stage5DrawData *draw_data = ARGS.draw_data;
	// rotates the stairs so the camera points downwards at an angle
	draw_data->stairs.omega_target = draw_data->stairs.omega - 0.4;
	WAIT(150);
	// stops rotation where it is
	draw_data->stairs.omega_target = draw_data->stairs.omega + 0.4;
}

TASK(animate_bg, { Stage5DrawData *draw_data; }) {
	Stage5DrawData *draw_data = ARGS.draw_data;
	float w = 0.005;
	YIELD;

	INVOKE_SUBTASK_DELAYED(6350, camera_down, draw_data);

	for(int t = 0;; t += WAIT(1)) {
		stage3d_update(&stage_3d_context);

		// rotates stairs around camera
		fapproach_p(&draw_data->stairs.omega, draw_data->stairs.omega_target, 0.1);

		draw_data->stairs.rotshift += draw_data->stairs.omega;
		stage_3d_context.cam.rot.v[0] += draw_data->stairs.omega * 0.5;
		draw_data->stairs.rad += draw_data->stairs.omega * 20;

		stage_3d_context.cam.pos[0] = draw_data->stairs.rad * cos(-w * global.frames);
		stage_3d_context.cam.pos[1] = draw_data->stairs.rad * sin(-w * global.frames);
		stage_3d_context.cam.pos[2] = -1700 + w * 3000/M_PI * global.frames;

		stage_3d_context.cam.rot.v[2] = draw_data->stairs.rotshift - 180/M_PI * w * global.frames;

		draw_data->stairs.light_strength *= 0.98;

		// lightning effect
		if(rng_chance(0.01)) {
			draw_data->stairs.light_strength = 5 + 5 * rng_real();
		}
	}
}

void stage5_bg_init_fullstage(void) {
	Stage5DrawData *draw_data = stage5_get_draw_data();

	stage_3d_context.cam.aspect = STAGE3D_DEFAULT_ASPECT; // FIXME
	stage_3d_context.cam.near = 100;
	stage_3d_context.cam.far = 20000;
	INVOKE_TASK(animate_bg, draw_data);
}

void stage5_bg_init_spellpractice(void) {
	Stage5DrawData *draw_data = stage5_get_draw_data();
	INVOKE_TASK(camera_down, draw_data);

	stage_3d_context.cam.aspect = STAGE3D_DEFAULT_ASPECT; // FIXME
	stage_3d_context.cam.near = 100;
	stage_3d_context.cam.far = 20000;
	INVOKE_TASK(animate_bg, draw_data);
}
