/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#ifndef IGUARD_resource_material_h
#define IGUARD_resource_material_h

#include "taisei.h"

#include "resource.h"
#include "texture.h"

typedef struct PBRMaterial {
	Texture *diffuse_map;
	Texture *normal_map;
	Texture *ambient_map;
	Texture *roughness_map;
	Texture *depth_map;

	vec3 diffuse_color;
	vec3 ambient_color;
	float roughness_value;
	float metallic_value;
	float depth_scale;
} PBRMaterial;

extern ResourceHandler material_res_handler;

#define MATERIAL_PATH_PREFIX "res/gfx/"
#define MATERIAL_EXTENSION ".material"

DEFINE_RESOURCE_GETTER(PBRMaterial, res_material, RES_MATERIAL)
DEFINE_OPTIONAL_RESOURCE_GETTER(PBRMaterial, res_material_optional, RES_MATERIAL)

#endif // IGUARD_resource_material_h
