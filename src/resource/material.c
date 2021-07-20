/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "material.h"

static char *material_path(const char *basename);
static bool material_check_path(const char *path);
static void material_load_stage1(ResourceLoadState *st);

ResourceHandler material_res_handler = {
	.type = RES_MATERIAL,
	.typename = "material",
	.subdir = MATERIAL_PATH_PREFIX,

	.procs = {
		.find = material_path,
		.check = material_check_path,
		.load = material_load_stage1,
		.unload = free,
	},
};

static char *material_path(const char *basename) {
	return try_path(MATERIAL_PATH_PREFIX, basename, MATERIAL_EXTENSION);
}

static bool material_check_path(const char *path) {
	return strendswith(path, MATERIAL_EXTENSION);
}

struct mat_load_data {
	PBRMaterial *mat;

	union {
		struct {
			char *diffuse_map;
			char *normal_map;
			char *ambient_map;
			char *roughness_map;
			char *depth_map;
		};

		char *maps[5];
	};
};

static void free_mat_load_data(struct mat_load_data *ld) {
	for(int i = 0; i < ARRAY_SIZE(ld->maps); ++i) {
		free(ld->maps[i]);
	}

	free(ld);
}

static void material_load_stage2(ResourceLoadState *st);

static void material_load_stage1(ResourceLoadState *st) {
	struct mat_load_data *ld = calloc(1, sizeof(*ld));
	ld->mat = calloc(1, sizeof(*ld->mat));
	*ld->mat = (PBRMaterial) {
		.diffuse_color = { 1, 1, 1 },
		.ambient_color = { 1, 1, 1 },
		.roughness_value = 1,
		.metallic_value = 0,
		.depth_scale = 0,
	};

	if(!parse_keyvalue_file_with_spec(st->path, (KVSpec[]) {
		{ "diffuse_map",      .out_str = &ld->diffuse_map },
		{ "normal_map",       .out_str = &ld->normal_map },
		{ "ambient_map",      .out_str = &ld->ambient_map },
		{ "roughness_map",    .out_str = &ld->roughness_map },
		{ "depth_map",        .out_str = &ld->depth_map },
		{ "diffuse_color",    .callback = kvparser_vec3, .callback_data = ld->mat->diffuse_color },
		{ "ambient_color",    .callback = kvparser_vec3, .callback_data = ld->mat->ambient_color },
		{ "roughness",        .out_float = &ld->mat->roughness_value },
		{ "metallic",         .out_float = &ld->mat->metallic_value },
		{ "depth_scale",      .out_float = &ld->mat->depth_scale },
		{ NULL },
	})) {
		free_mat_load_data(ld);
		log_error("Failed to parse material file '%s'", st->path);
		res_load_failed(st);
		return;
	}

	for(int i = 0; i < ARRAY_SIZE(ld->maps); ++i) {
		if(ld->maps[i]) {
			res_load_dependency(st, RES_TEXTURE, ld->maps[i]);
		}
	}

	res_load_continue_after_dependencies(st, material_load_stage2, ld);
}

#define LOADMAP(_map_) do { \
	if(ld->_map_##_map) { \
		ld->mat->_map_##_map = get_resource_data(RES_TEXTURE, ld->_map_##_map, st->flags); \
		if(UNLIKELY(ld->mat->_map_##_map == NULL)) { \
			log_fatal("%s: failed to load " #_map_ " map '%s'", st->name, ld->_map_##_map); \
			free_mat_load_data(ld); \
			res_load_failed(st); \
		} \
	} \
} while(0)

static void material_load_stage2(ResourceLoadState *st) {
	struct mat_load_data *ld = st->opaque;

	LOADMAP(diffuse);
	LOADMAP(normal);
	LOADMAP(ambient);
	LOADMAP(roughness);
	LOADMAP(depth);

	res_load_finished(st, ld->mat);
	free_mat_load_data(ld);
}
