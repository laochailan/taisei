/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#ifndef IGUARD_stages_stage2_draw_h
#define IGUARD_stages_stage2_draw_h

#include "taisei.h"

#include "stagedraw.h"
#include "stageutils.h"

typedef struct Stage2DrawData {
	struct {
		Color color;
	} fog;

	struct {
		PBRModel branch;
		PBRModel grass;
		PBRModel ground;
		PBRModel leaves;
		PBRModel rocks;
	} models;

	real hina_lights;
} Stage2DrawData;

Stage2DrawData *stage2_get_draw_data(void)
	attr_returns_nonnull attr_returns_max_aligned;

void stage2_drawsys_init(void);
void stage2_drawsys_shutdown(void);
void stage2_draw(void);

extern ShaderRule stage2_bg_effects[];

#endif // IGUARD_stages_stage2_draw_h
