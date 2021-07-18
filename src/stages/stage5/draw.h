/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_stages_stage5_draw_h
#define IGUARD_stages_stage5_draw_h

#include "taisei.h"

#include "stagedraw.h"

typedef struct Stage5DrawData {

	struct {
		float exponent;
		float brightness;
	} fog;

	struct {
		float r;
		float g;
		float b;
		float mixfactor;
	} color;

	struct {
		float light_strength;

		float rotshift;
		float rad;

		float omega;
		float omega_target;
	} stairs;

} Stage5DrawData;

void stage5_drawsys_init(void);
void stage5_drawsys_shutdown(void);
void stage5_draw(void);

void iku_spell_bg(Boss *b, int t);

Stage5DrawData *stage5_get_draw_data(void);

extern ShaderRule stage5_bg_effects[];
extern ShaderRule stage5_postprocess[];

#endif // IGUARD_stages_stage5_draw_h
