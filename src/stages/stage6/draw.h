/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_stages_stage6_draw_h
#define IGUARD_stages_stage6_draw_h

#include "taisei.h"

#include "util/fbpair.h"
#include "stagedraw.h"
#include "stageutils.h"

enum {
	NUM_STARS = 400
};

typedef struct Stage6DrawData {
	struct {
		vec3 position[NUM_STARS];
	} stars;

	struct {
		float angular_velocity;
		float radius;
	} stair_ascent;

	struct {
		Framebuffer *aux_fb;
		FBPair fbpair;
	} baryon;

	struct {
		PBRModel rim;
		PBRModel spires;
		PBRModel stairs;
		PBRModel tower;
		PBRModel tower_bottom;

		// these don't use pbr materials
		Model *calabi_yau_quintic;
		Model *top_plate;
	} models;

	Texture *envmap;
} Stage6DrawData;

extern ShaderRule stage6_bg_effects[];

Stage6DrawData* stage6_get_draw_data(void);

void stage6_drawsys_init(void);
void stage6_drawsys_shutdown(void);
void stage6_draw(void);

void baryon_center_draw(Enemy*, int, bool);
void baryon(Enemy*, int, bool);

extern ShaderRule stage6_bg_effects[];
extern ShaderRule stage6_postprocess[];

void elly_spellbg_toe(Boss*, int);
void elly_spellbg_modern_dark(Boss*, int);
void elly_spellbg_modern(Boss*, int);
void elly_spellbg_classic(Boss*, int);
void elly_global_rule(Boss*, int);

#endif // IGUARD_stages_stage6_draw_h
