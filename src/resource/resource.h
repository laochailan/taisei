/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef RESOURCE_H
#define RESOURCE_H

#include "global.h"

#include "texture.h"
#include "animation.h"
#include "audio.h"
#include "bgm.h"
#include "shader.h"
#include "font.h"
#include "model.h"

typedef struct Resources Resources;

typedef enum ResourceState {
	RS_GfxLoaded = 1,
	RS_SfxLoaded = 2,
	RS_ShaderLoaded = 4,
	RS_ModelsLoaded = 8,
	RS_BgmLoaded = 16,
	RS_FontsLoaded = 32,
} ResourceState;

struct Resources {
	ResourceState state;

	Texture *textures;
	Animation *animations;
	Sound *sounds;
	Sound *music;
	Shader *shaders;
	Model *models;
	Bgm_desc *bgm_descriptions;

	FBO fbg[2];
	FBO fsec;
};

extern Resources resources;

void load_resources(void);
void free_resources(void);

void draw_loading_screen(void);
#endif
