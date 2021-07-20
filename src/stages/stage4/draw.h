/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_stages_stage4_draw_h
#define IGUARD_stages_stage4_draw_h

#include "taisei.h"

#include "stagedraw.h"
#include "color.h"
#include "stageutils.h"

typedef struct Stage4DrawData {
	Color ambient_color;
	vec3 midboss_light_pos;

	struct {
		PBRModel corridor;
		PBRModel ground;
		PBRModel mansion;
	} models;
} Stage4DrawData;

void stage4_drawsys_init(void);
void stage4_drawsys_shutdown(void);

Stage4DrawData *stage4_get_draw_data(void)
	attr_returns_nonnull attr_returns_max_aligned;

void stage4_draw(void);

extern ShaderRule stage4_bg_effects[];

#endif // IGUARD_stages_stage4_draw_h
