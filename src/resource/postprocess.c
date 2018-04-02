/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include <SDL.h>
#include "util.h"
#include "postprocess.h"
#include "resource.h"
#include "renderer/api.h"

ResourceHandler postprocess_res_handler = {
	.type = RES_POSTPROCESS,
	.typename = "postprocessing pipeline",
	.subdir = PP_PATH_PREFIX,

	.procs = {
		.find = postprocess_path,
		.check = check_postprocess_path,
		.begin_load = load_postprocess_begin,
		.end_load = load_postprocess_end,
		.unload = unload_postprocess,
	},
};

typedef struct PostprocessLoadData {
	PostprocessShader *list;
	ResourceFlags resflags;
} PostprocessLoadData;

static bool postprocess_load_callback(const char *key, const char *value, void *data) {
	PostprocessLoadData *ldata = data;
	PostprocessShader **slist = &ldata->list;
	PostprocessShader *current = *slist;

	if(!strcmp(key, "@shader")) {
		current = malloc(sizeof(PostprocessShader));
		current->uniforms = NULL;

		// if loading this fails, get_resource will print a warning
		current->shader = get_resource_data(RES_SHADER_PROGRAM, value, ldata->resflags);

		list_append(slist, current);
		return true;
	}

	for(PostprocessShader *c = current; c; c = c->next) {
		current = c;
	}

	if(!current) {
		log_warn("Uniform '%s' ignored: no active shader", key);
		return true;
	}

	if(!current->shader) {
		// If loading the shader failed, just discard the uniforms.
		// We will get rid of empty shader definitions later.
		return true;
	}

	const char *name = key;
	Uniform *uni = r_shader_uniform(current->shader, name);

	if(!uni) {
		log_warn("No active uniform '%s' in shader", name);
		return true;
	}

	UniformType type = r_uniform_type(uni);
	const UniformTypeInfo *type_info = r_uniform_type_info(type);

	bool integer_type;

	switch(type) {
		case UNIFORM_INT:
		case UNIFORM_IVEC2:
		case UNIFORM_IVEC3:
		case UNIFORM_IVEC4:
		case UNIFORM_SAMPLER:
			integer_type = true;
			break;

		default:
			integer_type = false;
			break;
	}

	uint array_size = 0;

	PostprocessShaderUniformValue *values = NULL;
	assert(sizeof(*values) == type_info->element_size);
	uint val_idx = 0;

	while(true) {
		PostprocessShaderUniformValue v;
		char *next = (char*)value;

		if(integer_type) {
			v.i = strtol(value, &next, 10);
		} else {
			v.f = strtof(value, &next);
		}

		if(value == next) {
			break;
		}

		value = next;
		values = realloc(values, (++array_size) * type_info->elements * type_info->element_size);
		values[val_idx] = v;
		val_idx++;

		for(int i = 1; i < type_info->elements; ++i, ++val_idx) {
			PostprocessShaderUniformValue *v = values + val_idx;

			if(integer_type) {
				v->i = strtol(value, (char**)&value, 10);
			} else {
				v->f = strtof(value, (char**)&value);
			}
		}
	}

	if(val_idx == 0) {
		log_warn("No values defined for uniform '%s'", name);
		return true;
	}

	assert(val_idx % type_info->elements == 0);

	PostprocessShaderUniform *psu = calloc(1, sizeof(PostprocessShaderUniform));
	psu->uniform = uni;
	psu->values = values;
	psu->elements = val_idx / type_info->elements;

	list_append(&current->uniforms, psu);

	for(int i = 0; i < val_idx; ++i) {
		log_debug("u[%i] = (f: %f; i: %i)", i, psu->values[i].f, psu->values[i].i);
	}

	return true;
}

static void* delete_uniform(List **dest, List *data, void *arg) {
	PostprocessShaderUniform *uni = (PostprocessShaderUniform*)data;
	free(uni->values);
	free(list_unlink(dest, data));
	return NULL;
}

static void* delete_shader(List **dest, List *data, void *arg) {
	PostprocessShader *ps = (PostprocessShader*)data;
	list_foreach(&ps->uniforms, delete_uniform, NULL);
	free(list_unlink(dest, data));
	return NULL;
}

PostprocessShader* postprocess_load(const char *path, ResourceFlags flags) {
	PostprocessLoadData *ldata = calloc(1, sizeof(PostprocessLoadData));
	ldata->resflags = flags;
	parse_keyvalue_file_cb(path, postprocess_load_callback, ldata);
	PostprocessShader *list = ldata->list;
	free(ldata);

	for(PostprocessShader *s = list, *next; s; s = next) {
		next = s->next;

		if(!s->shader) {
			delete_shader((List**)&list, (List*)s, NULL);
		}
	}

	return list;
}

void postprocess_unload(PostprocessShader **list) {
	list_foreach(list, delete_shader, NULL);
}

void postprocess(PostprocessShader *ppshaders, FBOPair *fbos, PostprocessPrepareFuncPtr prepare, PostprocessDrawFuncPtr draw) {
	if(!ppshaders) {
		return;
	}

	ShaderProgram *shader_saved = r_shader_current();
	BlendMode blend_saved = r_blend_current();

	r_blend(BLEND_NONE);

	for(PostprocessShader *pps = ppshaders; pps; pps = pps->next) {
		ShaderProgram *s = pps->shader;

		r_target(fbos->back);
		r_shader_ptr(s);

		if(prepare) {
			prepare(fbos->back, s);
		}

		for(PostprocessShaderUniform *u = pps->uniforms; u; u = u->next) {
			r_uniform_ptr(u->uniform, u->elements, u->values);
		}

		draw(fbos->front);
		swap_fbo_pair(fbos);
	}

	r_shader_ptr(shader_saved);
	r_blend(blend_saved);
}

/*
 *  Glue for resources api
 */

char* postprocess_path(const char *name) {
	return strjoin(PP_PATH_PREFIX, name, PP_EXTENSION, NULL);
}

bool check_postprocess_path(const char *path) {
	return strendswith(path, PP_EXTENSION);
}

void* load_postprocess_begin(const char *path, uint flags) {
	return (void*)true;
}

void* load_postprocess_end(void *opaque, const char *path, uint flags) {
	return postprocess_load(path, flags);
}

void unload_postprocess(void *vlist) {
	postprocess_unload((PostprocessShader**)&vlist);
}
