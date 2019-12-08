/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "framerate.h"
#include "global.h"
#include "video.h"

void fpscounter_reset(FPSCounter *fps) {
	hrtime_t frametime = HRTIME_RESOLUTION / FPS;
	const int log_size = sizeof(fps->frametimes)/sizeof(hrtime_t);

	for(int i = 0; i < log_size; ++i) {
		fps->frametimes[i] = frametime;
	}

	fps->fps = HRTIME_RESOLUTION / (float64x)frametime;
	fps->frametime = frametime;
	fps->last_update_time = time_get();
}

void fpscounter_update(FPSCounter *fps) {
	const int log_size = sizeof(fps->frametimes)/sizeof(hrtime_t);
	hrtime_t frametime = time_get() - fps->last_update_time;

	memmove(fps->frametimes, fps->frametimes + 1, (log_size - 1) * sizeof(hrtime_t));
	fps->frametimes[log_size - 1] = frametime;

	hrtime_t avg = 0;

	for(int i = 0; i < log_size; ++i) {
		avg += fps->frametimes[i];
	}

	fps->fps = HRTIME_RESOLUTION / (avg / (float64x)log_size);
	fps->frametime = avg / log_size;
	fps->last_update_time = time_get();
}

uint32_t get_effective_frameskip(void) {
	uint32_t frameskip;

	if(global.frameskip > 0) {
		frameskip = global.frameskip;
	} else {
		frameskip = config_get_int(CONFIG_VID_FRAMESKIP);
	}

	return frameskip;
}
