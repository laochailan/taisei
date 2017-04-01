/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "resource.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "paths/native.h"
#include "config.h"
#include "video.h"
#include "menu/mainmenu.h"

Resources resources;
static SDL_threadID main_thread_id;

static const char *resource_type_names[] = {
	"texture",
	"animation",
	"sound",
	"bgm",
	"shader",
	"model",
};

static inline ResourceHandler* get_handler(ResourceType type) {
	return resources.handlers + type;
}

static uint32_t sdlevent_finalize_load;

static void register_handler(
	ResourceType type,
	const char *subdir,	// trailing slash assumed
	ResourceBeginLoadFunc begin_load,
	ResourceEndLoadFunc end_load,
	ResourceUnloadFunc unload,
	ResourceNameFunc name,
	ResourceFindFunc find,
	ResourceCheckFunc check,
	size_t tablesize)
{
	assert(type >= 0 && type < RES_NUMTYPES);

	ResourceHandler *h = get_handler(type);
	h->type = type;
	h->begin_load = begin_load;
	h->end_load = end_load;
	h->unload = unload;
	h->name = name;
	h->find = find;
	h->check = check;
	h->mapping = hashtable_new_stringkeys(tablesize);
	h->async_load_data = hashtable_new_stringkeys(tablesize);
	strcpy(h->subdir, subdir);
}

static void unload_resource(Resource *res) {
	get_handler(res->type)->unload(res->data);
	free(res);
}

Resource* insert_resource(ResourceType type, const char *name, void *data, ResourceFlags flags, const char *source) {
	assert(name != NULL);
	assert(source != NULL);

	ResourceHandler *handler = get_handler(type);
	Resource *oldres = hashtable_get_string(handler->mapping, name);
	Resource *res = malloc(sizeof(Resource));

	if(type == RES_MODEL) {
		// FIXME: models can't be safely unloaded at runtime
		flags |= RESF_PERMANENT;
	}

	res->type = handler->type;
	res->flags = flags;
	res->data = data;

	if(oldres) {
		log_warn("Replacing a previously loaded %s '%s'", resource_type_names[type], name);
		unload_resource(oldres);
	}

	hashtable_set_string(handler->mapping, name, res);

	log_info("Loaded %s '%s' from '%s' (%s)", resource_type_names[handler->type], name, source,
		(flags & RESF_PERMANENT) ? "permanent" : "transient");

	return res;
}

static char* get_name(ResourceHandler *handler, const char *path) {
	if(handler->name) {
		return handler->name(path);
	}

	return resource_util_basename(handler->subdir, path);
}

typedef struct ResourceAsyncLoadData {
	ResourceHandler *handler;
	char *path;
	char *name;
	ResourceFlags flags;
	void *opaque;
} ResourceAsyncLoadData;

static int load_resource_async_thread(void *vdata) {
	ResourceAsyncLoadData *data = vdata;
	SDL_Event evt;

	SDL_SetThreadPriority(SDL_THREAD_PRIORITY_LOW);
	data->opaque = data->handler->begin_load(data->path, data->flags);

	SDL_zero(evt);
	evt.type = sdlevent_finalize_load;
	evt.user.data1 = data;
	SDL_PushEvent(&evt);

	log_debug("Thread is exiting");
	return 0;
}

static Resource* load_resource_finish(void *opaque, ResourceHandler *handler, const char *path, const char *name, char *allocated_path, char *allocated_name, ResourceFlags flags);

bool resource_sdl_event(SDL_Event *evt) {
	assert(SDL_ThreadID() == main_thread_id);

	if(evt->type != sdlevent_finalize_load) {
		return false;
	}

	ResourceAsyncLoadData *data = evt->user.data1;

	if(!data) {
		return true;
	}

	char name[strlen(data->name) + 1];
	strcpy(name, data->name);

	load_resource_finish(data->opaque, data->handler, data->path, data->name, data->path, data->name, data->flags);
	hashtable_unset(data->handler->async_load_data, name);
	free(data);

	return true;
}

static void load_resource_async(ResourceHandler *handler, char *path, char *name, ResourceFlags flags) {
	log_debug("Loading %s '%s' asynchronously", resource_type_names[handler->type], name);

	ResourceAsyncLoadData *data = malloc(sizeof(ResourceAsyncLoadData));
	hashtable_set_string(handler->async_load_data, name, data);

	data->handler = handler;
	data->path = path;
	data->name = name;
	data->flags = flags;

	SDL_Thread *thread = SDL_CreateThread(load_resource_async_thread, __func__, data);

	if(thread) {
		SDL_DetachThread(thread);
	} else {
		log_warn("SDL_CreateThread() failed: %s", SDL_GetError());
		log_warn("Falling back to synchronous loading. Use TAISEI_NOASYNC=1 to suppress this warning.");
		load_resource_async_thread(data);
	}

}

static void update_async_load_state(void) {
	SDL_Event evt;
	while(SDL_PeepEvents(&evt, 1, SDL_GETEVENT, sdlevent_finalize_load, sdlevent_finalize_load)) {
		resource_sdl_event(&evt);
	}
}

static bool resource_check_async_load(ResourceHandler *handler, const char *name) {
	if(SDL_ThreadID() == main_thread_id)
		update_async_load_state();
	ResourceAsyncLoadData *data = hashtable_get_string(handler->async_load_data, name);
	return data;
}

static void resource_wait_for_async_load(ResourceHandler *handler, const char *name) {
	while(resource_check_async_load(handler, name));
}

static void resource_wait_for_all_async_loads(ResourceHandler *handler) {
	char *key;

	hashtable_lock(handler->async_load_data);
	HashtableIterator *i = hashtable_iter(handler->async_load_data);
	while(hashtable_iter_next(i, (void**)&key, NULL)) {
		resource_check_async_load(handler, key);
	}
	hashtable_unlock(handler->async_load_data);
}

static Resource* load_resource(ResourceHandler *handler, const char *path, const char *name, ResourceFlags flags, bool async) {
	Resource *res;

	const char *loltypename = resource_type_names[handler->type];
	char *allocated_path = NULL;
	char *allocated_name = NULL;

	assert(path || name);

	if(!path) {
		path = allocated_path = handler->find(name);

		if(!path) {
			if(!(flags & RESF_OPTIONAL)) {
				log_fatal("Required %s '%s' couldn't be located", loltypename, name);
			} else {
				log_warn("Failed to locate %s '%s'", loltypename, name);
			}

			return NULL;
		}
	} else if(!name) {
		name = allocated_name = get_name(handler, path);
	}

	assert(handler->check(path));

	if(async) {
		if(resource_check_async_load(handler, name)) {
			return NULL;
		}
	} else {
		resource_wait_for_async_load(handler, name);
	}

	res = hashtable_get_string(handler->mapping, name);

	if(res) {
		log_warn("%s '%s' is already loaded", loltypename, name);
		free(allocated_name);
		return res;
	}

	if(async) {
		// these will be freed when loading is done
		path = allocated_path ? allocated_path : strdup(path);
		name = allocated_name ? allocated_name : strdup(name);
		load_resource_async(handler, (char*)path, (char*)name, flags);
		return NULL;
	}

	return load_resource_finish(handler->begin_load(path, flags), handler, path, name, allocated_path, allocated_name, flags);
}

static Resource* load_resource_finish(void *opaque, ResourceHandler *handler, const char *path, const char *name, char *allocated_path, char *allocated_name, ResourceFlags flags) {
	const char *loltypename = resource_type_names[handler->type];
	void *raw = handler->end_load(opaque, path, flags);

	if(!raw) {
		name = name ? name : "<name unknown>";
		path = path ? path : "<path unknown>";

		if(!(flags & RESF_OPTIONAL)) {
			log_fatal("Required %s '%s' couldn't be loaded (%s)", loltypename, name, path);
		} else {
			log_warn("Failed to load %s '%s' (%s)", loltypename, name, path);
		}

		free(allocated_path);
		free(allocated_name);

		return NULL;
	}

	Resource *res = insert_resource(handler->type, name, raw, flags, path);

	free(allocated_path);
	free(allocated_name);

	return res;
}

Resource* get_resource(ResourceType type, const char *name, ResourceFlags flags) {
	ResourceHandler *handler = get_handler(type);
	resource_wait_for_async_load(handler, name);
	Resource *res = hashtable_get_string(handler->mapping, name);

	if(!res) {
		if(!(flags & RESF_PRELOAD)) {
			log_warn("%s '%s' was not preloaded", resource_type_names[type], name);

			if(!(flags & RESF_OPTIONAL) && getenvint("TAISEI_PRELOAD_REQUIRED")) {
				log_fatal("Aborting due to TAISEI_PRELOAD_REQUIRED");
			}
		}

		res = load_resource(handler, NULL, name, flags, false);
	}

	if(res && flags & RESF_PERMANENT && !(res->flags & RESF_PERMANENT)) {
		log_debug("Promoted %s '%s' to permanent", resource_type_names[type], name);
		res->flags |= RESF_PERMANENT;
	}

	return res;
}

void preload_resource(ResourceType type, const char *name, ResourceFlags flags) {
	if(getenvint("TAISEI_NOPRELOAD"))
		return;

	ResourceHandler *handler = get_handler(type);

	if(hashtable_get_string(handler->mapping, name) ||
		hashtable_get_string(handler->async_load_data, name)) {
		return;
	}

	load_resource(handler, NULL, name, flags | RESF_PRELOAD, !getenvint("TAISEI_NOASYNC"));
}

void preload_resources(ResourceType type, ResourceFlags flags, const char *firstname, ...) {
	va_list args;
	va_start(args, firstname);

	for(const char *name = firstname; name; name = va_arg(args, const char*)) {
		preload_resource(type, name, flags);
	}

	va_end(args);
}

void init_resources(void) {
	register_handler(
		RES_TEXTURE, TEX_PATH_PREFIX, load_texture_begin, load_texture_end, (ResourceUnloadFunc)free_texture, NULL, texture_path, check_texture_path, HT_DYNAMIC_SIZE
	);

	register_handler(
		RES_ANIM, ANI_PATH_PREFIX, load_animation_begin, load_animation_end, free, NULL, animation_path, check_animation_path, HT_DYNAMIC_SIZE
	);

	register_handler(
		RES_SHADER, SHA_PATH_PREFIX, load_shader_begin, load_shader_end, unload_shader, NULL, shader_path, check_shader_path, HT_DYNAMIC_SIZE
	);

	register_handler(
		RES_MODEL, MDL_PATH_PREFIX, load_model_begin, load_model_end, unload_model, NULL, model_path, check_model_path, HT_DYNAMIC_SIZE
	);

	register_handler(
		RES_SFX, SFX_PATH_PREFIX, load_sound_begin, load_sound_end, unload_sound, NULL, sound_path, check_sound_path, HT_DYNAMIC_SIZE
	);

	register_handler(
		RES_BGM, BGM_PATH_PREFIX, load_music_begin, load_music_end, unload_music, NULL, music_path, check_music_path, HT_DYNAMIC_SIZE
	);

	main_thread_id = SDL_ThreadID();
}

void resource_util_strip_ext(char *path) {
	char *dot = strrchr(path, '.');
	if(dot > strrchr(path, '/'))
		*dot = 0;
}

char* resource_util_basename(const char *prefix, const char *path) {
	assert(strstartswith(path, prefix));

	char *out = strdup(path + strlen(prefix));
	resource_util_strip_ext(out);

	return out;
}

const char* resource_util_filename(const char *path) {
	char *sep = strrchr(path, '/');

	if(sep) {
		return sep + 1;
	}

	return path;
}

void load_resources(void) {
	init_fonts();

	if(glext.draw_instanced) {
		load_shader_snippets("shader/laser_snippets", "laser_", RESF_PERMANENT);
	}

	menu_preload();

	init_fbo(&resources.fbg[0]);
	init_fbo(&resources.fbg[1]);
	init_fbo(&resources.fsec);

	resources.stage_postprocess = postprocess_load("shader/postprocess.conf");
}

void free_resources(bool all) {
	for(ResourceType type = 0; type < RES_NUMTYPES; ++type) {
		ResourceHandler *handler = get_handler(type);

		resource_wait_for_all_async_loads(handler);

		char *name;
		Resource *res;
		ListContainer *unset_list = NULL;

		for(HashtableIterator *i = hashtable_iter(handler->mapping); hashtable_iter_next(i, (void**)&name, (void**)&res);) {
			if(!all && res->flags & RESF_PERMANENT)
				continue;

			ResourceFlags flags = res->flags;
			unload_resource(res);
			log_info("Unloaded %s '%s' (%s)", resource_type_names[type], name,
				(flags & RESF_PERMANENT) ? "permanent" : "transient"
			);

			if(!all) {
				hashtable_unset_deferred(handler->mapping, name, &unset_list);
			}
		}

		if(all) {
			hashtable_free(handler->mapping);
		} else {
			hashtable_unset_deferred_now(handler->mapping, &unset_list);
		}
	}

	if(!all) {
		return;
	}

	delete_vbo(&_vbo);

	postprocess_unload(&resources.stage_postprocess);

	delete_fbo(&resources.fbg[0]);
	delete_fbo(&resources.fbg[1]);
	delete_fbo(&resources.fsec);

	free_fonts();
}

void print_resource_hashtables(void) {
	for(ResourceType type = 0; type < RES_NUMTYPES; ++type) {
		hashtable_print_stringkeys(get_handler(type)->mapping);
	}
}
