/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "resource.h"
#include "config.h"
#include "video.h"
#include "menu/mainmenu.h"
#include "events.h"
#include "taskmanager.h"

#include "animation.h"
#include "bgm.h"
#include "font.h"
#include "material.h"
#include "model.h"
#include "postprocess.h"
#include "sfx.h"
#include "shader_object.h"
#include "shader_program.h"
#include "sprite.h"
#include "texture.h"

#include "renderer/common/backend.h"

ResourceHandler *_handlers[] = {
	[RES_ANIM]              = &animation_res_handler,
	[RES_BGM]               = &bgm_res_handler,
	[RES_FONT]              = &font_res_handler,
	[RES_MATERIAL]          = &material_res_handler,
	[RES_MODEL]             = &model_res_handler,
	[RES_POSTPROCESS]       = &postprocess_res_handler,
	[RES_SFX]               = &sfx_res_handler,
	[RES_SHADER_OBJECT]     = &shader_object_res_handler,
	[RES_SHADER_PROGRAM]    = &shader_program_res_handler,
	[RES_SPRITE]            = &sprite_res_handler,
	[RES_TEXTURE]           = &texture_res_handler,
};

typedef enum ResourceStatus {
	RES_STATUS_LOADING,
	RES_STATUS_LOADED,
	RES_STATUS_FAILED,
} ResourceStatus;

typedef enum LoadStatus {
	LOAD_NONE,
	LOAD_OK,
	LOAD_FAILED,
	LOAD_CONT_ON_MAIN,
	LOAD_CONT,
} LoadStatus;

typedef struct InternalResource InternalResource;
typedef struct InternalResLoadState InternalResLoadState;

struct InternalResource {
	Resource res;
	SDL_mutex *mutex;
	SDL_cond *cond;
	InternalResLoadState *load;
	ResourceStatus status;
};

struct InternalResLoadState {
	ResourceLoadState st;
	InternalResource *ires;
	Task *async_task;
	char *allocated_name;
	char *allocated_path;
	ResourceLoadProc continuation;
	DYNAMIC_ARRAY(InternalResource*) dependencies;
	LoadStatus status;
	bool ready_to_finalize;
};

static struct {
	hrtime_t frame_threshold;
	uchar loaded_this_frame : 1;
	struct {
		uchar no_async_load : 1;
		uchar no_preload : 1;
		uchar no_unload : 1;
		uchar preload_required : 1;
	} env;
} res_gstate;

static inline InternalResLoadState *loadstate_internal(ResourceLoadState *st) {
	return UNION_CAST(ResourceLoadState*, InternalResLoadState*, st);
}

static InternalResource *preload_resource_internal(ResourceType type, const char *name, ResourceFlags flags);

void res_load_failed(ResourceLoadState *st) {
	InternalResLoadState *ist = loadstate_internal(st);
	ist->status = LOAD_FAILED;
}

void res_load_finished(ResourceLoadState *st, void *res) {
	InternalResLoadState *ist = loadstate_internal(st);
	ist->status = LOAD_OK;
	ist->st.opaque = res;
}

void res_load_continue_on_main(ResourceLoadState *st, ResourceLoadProc callback, void *opaque) {
	InternalResLoadState *ist = loadstate_internal(st);
	ist->status = LOAD_CONT_ON_MAIN;
	ist->st.opaque = opaque;
	ist->continuation = callback;
}

void res_load_continue_after_dependencies(ResourceLoadState *st, ResourceLoadProc callback, void *opaque) {
	InternalResLoadState *ist = loadstate_internal(st);
	ist->status = LOAD_CONT;
	ist->st.opaque = opaque;
	ist->continuation = callback;
}

void res_load_dependency(ResourceLoadState *st, ResourceType type, const char *name) {
	InternalResLoadState *ist = loadstate_internal(st);
	InternalResource *dep = preload_resource_internal(type, name, st->flags);
	InternalResource *ires = ist->ires;
	SDL_LockMutex(ires->mutex);
	*dynarray_append(&ist->dependencies) = dep;
	SDL_UnlockMutex(ires->mutex);
}

static inline ResourceHandler *get_handler(ResourceType type) {
	return *(_handlers + type);
}

static inline ResourceHandler *get_ires_handler(InternalResource *ires) {
	return get_handler(ires->res.type);
}

static void alloc_handler(ResourceHandler *h) {
	assert(h != NULL);
	ht_create(&h->private.mapping);
}

static const char *type_name(ResourceType type) {
	return get_handler(type)->typename;
}

struct valfunc_arg {
	ResourceType type;
};

static void *valfunc_begin_load_resource(void* arg) {
	ResourceType type = ((struct valfunc_arg*)arg)->type;

	InternalResource *ires = calloc(1, sizeof(InternalResource));
	ires->res.type = type;
	ires->status = RES_STATUS_LOADING;
	ires->mutex = SDL_CreateMutex();
	ires->cond = SDL_CreateCond();

	return ires;
}

static bool try_begin_load_resource(ResourceType type, const char *name, hash_t hash, InternalResource **out_ires) {
	ResourceHandler *handler = get_handler(type);
	struct valfunc_arg arg = { type };
	return ht_try_set_prehashed(&handler->private.mapping, name, hash, &arg, valfunc_begin_load_resource, (void**)out_ires);
}

static void load_resource_finish(InternalResLoadState *st);

static ResourceStatus pump_or_wait_for_dependencies(InternalResLoadState *st, bool pump_only);

static ResourceStatus pump_dependencies(InternalResLoadState *st) {
	return pump_or_wait_for_dependencies(st, true);
}

static ResourceStatus wait_for_dependencies(InternalResLoadState *st) {
	return pump_or_wait_for_dependencies(st, false);
}

static ResourceStatus pump_or_wait_for_resource_load(InternalResource *ires, uint32_t want_flags, bool pump_only) {
	SDL_LockMutex(ires->mutex);

	InternalResLoadState *load_state = ires->load;

	if(load_state) {
		assert(ires->status == RES_STATUS_LOADING);

		Task *task = load_state->async_task;

		if(task) {
			// If there's an async load task for this resource, wait for it for complete.
			// If it's not yet running, it will be offloaded to this thread instead.

			load_state->async_task = NULL;
			SDL_UnlockMutex(ires->mutex);

			if(!task_finish(task, NULL)) {
				log_fatal("Internal error: async task failed");
			}

			SDL_LockMutex(ires->mutex);

			// It's important to refresh load_state here, because the task may have finalized the load.
			// This may happen if the resource doesn't need to be finalized on the main thread, or if we *are* the main thread.
			load_state = ires->load;
		}

		if(load_state) {
			ResourceStatus dep_status = pump_dependencies(load_state);

			if(load_state->ready_to_finalize && is_main_thread()) {
				// Resource has finished async load, but needs to be finalized on the main thread.
				// Since we are the main thread, we can do it here.

				if(dep_status == RES_STATUS_LOADING) {
					dep_status = wait_for_dependencies(load_state);
				}

				load_resource_finish(load_state);
				assert(ires->status != RES_STATUS_LOADING);
			}
		}
	}

	while(ires->status == RES_STATUS_LOADING && !pump_only) {
		// If we get to this point, then we're waiting for a resource that's awaiting finalization on the main thread.
		// If we *are* the main thread, there's no excuse for us to wait; we should've finalized the resource ourselves.
		assert(!is_main_thread());
		SDL_CondWait(ires->cond, ires->mutex);
	}

	ResourceStatus status = ires->status;

	if(status == RES_STATUS_LOADED) {
		uint32_t missing_flags = want_flags & ~ires->res.flags;

		if(missing_flags) {
			uint32_t new_flags = ires->res.flags | want_flags;
			log_debug("Flags for %s at %p promoted from 0x%08x to 0x%08x", type_name(ires->res.type), (void*)ires, ires->res.flags, new_flags);
			ires->res.flags = new_flags;
		}
	}

	SDL_UnlockMutex(ires->mutex);

	return status;
}

static ResourceStatus wait_for_resource_load(InternalResource *ires, uint32_t want_flags) {
	return pump_or_wait_for_resource_load(ires, want_flags, false);
}

static ResourceStatus pump_or_wait_for_dependencies(InternalResLoadState *st, bool pump_only) {
	ResourceStatus dep_status = RES_STATUS_LOADED;

	dynarray_foreach_elem(&st->dependencies, InternalResource **dep, {
		ResourceStatus s = pump_or_wait_for_resource_load(*dep, st->st.flags, pump_only);

		switch(s) {
			case RES_STATUS_FAILED:
				return RES_STATUS_FAILED;

			case RES_STATUS_LOADING:
				dep_status = RES_STATUS_LOADING;
				break;

			case RES_STATUS_LOADED:
				break;

			default:
				UNREACHABLE;
		}
	});

	if(!pump_only) {
		assert(dep_status != RES_STATUS_LOADING);
	}

	return dep_status;
}

static void *load_resource_async_task(void *vdata) {
	InternalResLoadState *st = vdata;
	InternalResource *ires = st->ires;
	assume(st == ires->load);

	SDL_LockMutex(ires->mutex);
	ResourceHandler *h = get_ires_handler(ires);

	st->status = LOAD_NONE;
	h->procs.load(&st->st);

retry:
	switch(st->status) {
		case LOAD_CONT: {
			ResourceStatus dep_status;

			if(is_main_thread()) {
				dep_status = pump_dependencies(st);

				if(dep_status == RES_STATUS_LOADING) {
					dep_status = wait_for_dependencies(st);
				}

				st->status = LOAD_NONE;
				st->continuation(&st->st);
				goto retry;
			} else {
				dep_status = pump_dependencies(st);

				if(dep_status == RES_STATUS_LOADING) {
					st->status = LOAD_CONT_ON_MAIN;
					st->ready_to_finalize = true;
					events_emit(TE_RESOURCE_ASYNC_LOADED, 0, ires, NULL);
					break;
				} else {
					st->status = LOAD_NONE;
					st->continuation(&st->st);
					goto retry;
				}
			}

			UNREACHABLE;
		}

		case LOAD_CONT_ON_MAIN:
			if(pump_dependencies(st) == RES_STATUS_LOADING || !is_main_thread()) {
				st->ready_to_finalize = true;
				events_emit(TE_RESOURCE_ASYNC_LOADED, 0, ires, NULL);
				break;
			}
		// fallthrough
		case LOAD_OK:
		case LOAD_FAILED:
			st->ready_to_finalize = true;
			load_resource_finish(st);
			st = NULL;
			break;

		default:
			UNREACHABLE;
	}

	assume(ires->load == st);
	SDL_UnlockMutex(ires->mutex);
	return st;
}

static SDLCALL int filter_asyncload_event(void *vires, SDL_Event *event) {
	return event->type == MAKE_TAISEI_EVENT(TE_RESOURCE_ASYNC_LOADED) && event->user.data1 == vires;
}

static void unload_resource(InternalResource *ires) {
	assert(is_main_thread());

	if(wait_for_resource_load(ires, 0) == RES_STATUS_LOADED) {
		get_handler(ires->res.type)->procs.unload(ires->res.data);
	}

	SDL_PumpEvents();
	SDL_FilterEvents(filter_asyncload_event, ires);

	SDL_DestroyCond(ires->cond);
	SDL_DestroyMutex(ires->mutex);
	free(ires);
}

static char *get_name_from_path(ResourceHandler *handler, const char *path) {
	if(!strstartswith(path, handler->subdir)) {
		return NULL;
	}

	return resource_util_basename(handler->subdir, path);
}

static bool should_defer_load(void) {
	FrameTimes ft = eventloop_get_frame_times();

	if(ft.next != res_gstate.frame_threshold) {
		res_gstate.frame_threshold = ft.next;
		res_gstate.loaded_this_frame = false;
	}

	if(!res_gstate.loaded_this_frame) {
		return false;
	}

	hrtime_t t = time_get();

	if(ft.next < t || ft.next - t < ft.target / 2) {
		return true;
	}

	return false;
}

static bool resource_asyncload_handler(SDL_Event *evt, void *arg) {
	assert(is_main_thread());

	InternalResource *ires = evt->user.data1;
	InternalResLoadState *st = ires->load;

	if(st == NULL) {
		return true;
	}

#if 1
	if(should_defer_load()) {
		events_defer(evt);
		return true;
	}
#endif

	SDL_LockMutex(ires->mutex);

	ResourceStatus dep_status = pump_dependencies(st);

	if(dep_status == RES_STATUS_LOADING) {
		log_debug("Deferring %s '%s' because some dependencies are not satisfied", type_name(ires->res.type), st->st.name);
		SDL_UnlockMutex(ires->mutex);
		events_defer(evt);
		return true;
	}

	Task *task = st->async_task;
	assert(!task || ires->status == RES_STATUS_LOADING);
	st->async_task = NULL;

	if(task) {
		SDL_UnlockMutex(ires->mutex);

		if(!task_finish(task, NULL)) {
			log_fatal("Internal error: async task failed");
		}

		SDL_LockMutex(ires->mutex);
		st = ires->load;
	}

	if(st) {
		load_resource_finish(ires->load);
		res_gstate.loaded_this_frame = true;
	}

	SDL_UnlockMutex(ires->mutex);
	return true;
}

static InternalResLoadState *make_persistent_loadstate(InternalResLoadState *st_transient) {
	if(st_transient->ires->load != NULL) {
		assert(st_transient->ires->load == st_transient);
		return st_transient;
	}

	InternalResLoadState *st = memdup(st_transient, sizeof(*st));

	assume(st->st.name != NULL);
	assume(st->st.path != NULL);

	if(!st->allocated_name) {
		st->st.name = st->allocated_name = strdup(st->st.name);
	}

	if(!st->allocated_path) {
		st->st.path = st->allocated_path = strdup(st->st.path);
	}

	st->ires->load = st;
	return st;
}

static void load_resource_async(InternalResLoadState *st_transient) {
	InternalResLoadState *st = make_persistent_loadstate(st_transient);
	st->async_task = taskmgr_global_submit((TaskParams) { load_resource_async_task, st });
}

attr_nonnull_all
static void load_resource(InternalResource *ires, const char *name, ResourceFlags flags, bool async) {
	ResourceHandler *handler = get_ires_handler(ires);
	const char *typename = type_name(handler->type);
	char *path = NULL;

	if(handler->type == RES_SFX || handler->type == RES_BGM) {
		// audio stuff is always optional.
		// loading may fail if the backend failed to initialize properly, even though the resource exists.
		// this is a less than ideal situation, but it doesn't render the game unplayable.
		flags |= RESF_OPTIONAL;
	}

	path = handler->procs.find(name);

	if(path) {
		assert(handler->procs.check(path));
	} else {
		if(!(flags & RESF_OPTIONAL)) {
			log_fatal("Required %s '%s' couldn't be located", typename, name);
		} else {
			log_error("Failed to locate %s '%s'", typename, name);
		}

		ires->status = RES_STATUS_FAILED;
	}

	InternalResLoadState st = {
		.ires = ires,
		.allocated_path = path,
		.st = {
			.name = name,
			.path = path,
			.flags = flags,
		},
	};

	if(ires->status == RES_STATUS_FAILED) {
		st.ready_to_finalize = true;
		load_resource_finish(&st);
	} else if(async) {
		load_resource_async(&st);
	} else {
		st.status = LOAD_NONE;
		handler->procs.load(&st.st);

		retry: switch(st.status) {
			case LOAD_OK:
			case LOAD_FAILED:
				st.ready_to_finalize = true;
				load_resource_finish(&st);
				break;
			case LOAD_CONT:
			case LOAD_CONT_ON_MAIN:
				wait_for_dependencies(&st);
				st.status = LOAD_NONE;
				st.continuation(&st.st);
				goto retry;
			default: UNREACHABLE;
		}
	}
}

static void load_resource_finish(InternalResLoadState *st) {
	void *raw = NULL;
	InternalResource *ires = st->ires;

	assert(ires->status == RES_STATUS_LOADING || ires->status == RES_STATUS_FAILED);
	assert(st->ready_to_finalize);

	if(ires->status != RES_STATUS_FAILED) {
		retry: switch(st->status) {
			case LOAD_CONT:
			case LOAD_CONT_ON_MAIN:
				st->status = LOAD_NONE;
				st->continuation(&st->st);
				goto retry;

			case LOAD_OK:
				raw = NOT_NULL(st->st.opaque);
				break;

			case LOAD_FAILED:
				break;

			default:
				UNREACHABLE;
		}
	}

	dynarray_free_data(&st->dependencies);

	const char *name = NOT_NULL(st->st.name);
	const char *path = st->st.path ? st->st.path : "<path unknown>";
	char *allocated_source = vfs_repr(path, true);
	const char *source = allocated_source ? allocated_source : path;
	const char *typename = type_name(ires->res.type);

	ires->res.flags = st->st.flags;
	ires->res.data = raw;

	if(ires->res.type == RES_MODEL || res_gstate.env.no_unload) {
		// FIXME: models can't be safely unloaded at runtime yet
		ires->res.flags |= RESF_PERMANENT;
	}

	if(raw) {
		ires->status = RES_STATUS_LOADED;
		log_info("Loaded %s '%s' from '%s' (%s)", typename, name, source, (ires->res.flags & RESF_PERMANENT) ? "permanent" : "transient");
	} else {
		ires->status = RES_STATUS_FAILED;

		if(ires->res.flags & RESF_OPTIONAL) {
			log_error("Failed to load %s '%s' from '%s'", typename, name, source);
		} else {
			log_fatal("Required %s '%s' couldn't be loaded from '%s'", typename, name, source);
		}
	}

	free(allocated_source);
	free(st->allocated_path);
	free(st->allocated_name);

	assume(!ires->load || ires->load == st);

	Task *async_task = st->async_task;
	if(async_task) {
		st->async_task = NULL;
		task_detach(async_task);
	}

	free(ires->load);
	ires->load = NULL;

	SDL_CondBroadcast(ires->cond);
	assert(ires->status != RES_STATUS_LOADING);
}

Resource *_get_resource(ResourceType type, const char *name, hash_t hash, ResourceFlags flags) {
	InternalResource *ires;
	Resource *res;

	if(try_begin_load_resource(type, name, hash, &ires)) {
		SDL_LockMutex(ires->mutex);

		if(!(flags & RESF_PRELOAD)) {
			log_warn("%s '%s' was not preloaded", type_name(type), name);

			if(res_gstate.env.preload_required) {
				log_fatal("Aborting due to TAISEI_PRELOAD_REQUIRED");
			}
		}

		load_resource(ires, name, flags, false);
		SDL_CondBroadcast(ires->cond);

		if(ires->status == RES_STATUS_FAILED) {
			res = NULL;
		} else {
			assert(ires->status == RES_STATUS_LOADED);
			assert(ires->res.data != NULL);
			res = &ires->res;
		}

		SDL_UnlockMutex(ires->mutex);
		return res;
	} else {
		uint32_t promotion_flags = flags & RESF_PERMANENT;
		ResourceStatus status = wait_for_resource_load(ires, promotion_flags);

		if(status == RES_STATUS_FAILED) {
			return NULL;
		}

		assert(status == RES_STATUS_LOADED);
		assert(ires->res.data != NULL);

		return &ires->res;
	}
}

void *_get_resource_data(ResourceType type, const char *name, hash_t hash, ResourceFlags flags) {
	Resource *res = _get_resource(type, name, hash, flags);

	if(res) {
		return res->data;
	}

	return NULL;
}

static InternalResource *preload_resource_internal(ResourceType type, const char *name, ResourceFlags flags) {
	InternalResource *ires;

	if(try_begin_load_resource(type, name, ht_str2ptr_hash(name), &ires)) {
		SDL_LockMutex(ires->mutex);
		load_resource(ires, name, flags, !res_gstate.env.no_async_load);
		SDL_UnlockMutex(ires->mutex);
	}

	return ires;
}

void preload_resource(ResourceType type, const char *name, ResourceFlags flags) {
	if(!res_gstate.env.no_preload) {
		preload_resource_internal(type, name, flags | RESF_PRELOAD);
	}
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
	res_gstate.env.no_async_load = env_get("TAISEI_NOASYNC", false);
	res_gstate.env.no_preload = env_get("TAISEI_NOPRELOAD", false);
	res_gstate.env.no_unload = env_get("TAISEI_NOUNLOAD", false);
	res_gstate.env.preload_required = env_get("TAISEI_PRELOAD_REQUIRED", false);

	for(int i = 0; i < RES_NUMTYPES; ++i) {
		ResourceHandler *h = get_handler(i);
		alloc_handler(h);

		if(h->procs.init != NULL) {
			h->procs.init();
		}
	}

	if(!res_gstate.env.no_async_load) {
		EventHandler h = {
			.proc = resource_asyncload_handler,
			.priority = EPRIO_SYSTEM,
			.event_type = MAKE_TAISEI_EVENT(TE_RESOURCE_ASYNC_LOADED),
		};

		events_register_handler(&h);
	}
}

void resource_util_strip_ext(char *path) {
	char *dot = strrchr(path, '.');
	char *psep = strrchr(path, '/');

	if(dot && (!psep || dot > psep)) {
		*dot = 0;
	}
}

char *resource_util_basename(const char *prefix, const char *path) {
	assert(strstartswith(path, prefix));

	char *out = strdup(path + strlen(prefix));
	resource_util_strip_ext(out);

	return out;
}

const char *resource_util_filename(const char *path) {
	char *sep = strrchr(path, '/');

	if(sep) {
		return sep + 1;
	}

	return path;
}

static void preload_path(const char *path, ResourceType type, ResourceFlags flags) {
	if(_handlers[type]->procs.check(path)) {
		char *name = get_name_from_path(_handlers[type], path);
		if(name) {
			preload_resource(type, name, flags);
			free(name);
		}
	}
}

static void *preload_shaders(const char *path, void *arg) {
	preload_path(path, RES_SHADER_PROGRAM, RESF_PERMANENT);
	return NULL;
}

static void *preload_all(const char *path, void *arg) {
	for(ResourceType t = 0; t < RES_NUMTYPES; ++t) {
		preload_path(path, t, RESF_PERMANENT | RESF_OPTIONAL);
	}

	return NULL;
}

void *resource_for_each(ResourceType type, void* (*callback)(const char *name, Resource *res, void *arg), void *arg) {
	ht_str2ptr_ts_iter_t iter;
	ht_iter_begin(&get_handler(type)->private.mapping, &iter);
	void *result = NULL;

	while(iter.has_data) {
		InternalResource *ires = iter.value;
		char *key = iter.key;

		ht_iter_next(&iter);

		if(ires->res.data == NULL) {
			continue;
		}

		if((result = callback(key, &ires->res, arg)) != NULL) {
			break;
		}
	}

	ht_iter_end(&iter);
	return result;
}

void load_resources(void) {
	for(uint i = 0; i < RES_NUMTYPES; ++i) {
		ResourceHandler *h = get_handler(i);
		assert(h != NULL);

		if(h->procs.post_init) {
			h->procs.post_init();
		}
	}

	menu_preload();

	if(env_get("TAISEI_PRELOAD_SHADERS", 0)) {
		log_info("Loading all shaders now due to TAISEI_PRELOAD_SHADERS");
		vfs_dir_walk(SHPROG_PATH_PREFIX, preload_shaders, NULL);
	}

	if(env_get("TAISEI_AGGRESSIVE_PRELOAD", 0)) {
		log_info("Attempting to load all resources now due to TAISEI_AGGRESSIVE_PRELOAD");
		vfs_dir_walk("res/", preload_all, NULL);
	}
}

void free_resources(bool all) {
	ht_str2ptr_ts_iter_t iter;

	for(ResourceType type = 0; type < RES_NUMTYPES; ++type) {
		ResourceHandler *handler = get_handler(type);

		ht_iter_begin(&handler->private.mapping, &iter);

		for(; iter.has_data; ht_iter_next(&iter)) {
			wait_for_resource_load(iter.value, 0);
		}

		ht_iter_end(&iter);
	}

	for(ResourceType type = 0; type < RES_NUMTYPES; ++type) {
		ResourceHandler *handler = get_handler(type);
		InternalResource *ires;
		ht_str2ptr_ts_key_list_t *unset_list = NULL, *unset_entry;

		ht_iter_begin(&handler->private.mapping, &iter);

		for(; iter.has_data; ht_iter_next(&iter)) {
			ires = iter.value;

			if(!all && (ires->res.flags & RESF_PERMANENT)) {
				continue;
			}

			unset_entry = calloc(1, sizeof(*unset_entry));
			unset_entry->key = iter.key;
			list_push(&unset_list, unset_entry);
		}

		ht_iter_end(&iter);

		for(ht_str2ptr_ts_key_list_t *c; (c = list_pop(&unset_list));) {
			char *tmp = c->key;
			char name[strlen(tmp) + 1];
			strcpy(name, tmp);

			ires = ht_get(&handler->private.mapping, name, NULL);
			assert(ires != NULL);

			attr_unused ResourceFlags flags = ires->res.flags;

			if(!all) {
				ht_unset(&handler->private.mapping, name);
			}

			unload_resource(ires);
			free(c);

			log_info(
				"Unloaded %s '%s' (%s)",
				type_name(type),
				name,
				(flags & RESF_PERMANENT) ? "permanent" : "transient"
			);
		}

		if(all) {
			if(handler->procs.shutdown != NULL) {
				handler->procs.shutdown();
			}

			ht_destroy(&handler->private.mapping);
		}
	}

	if(!all) {
		return;
	}

	if(!res_gstate.env.no_async_load) {
		events_unregister_handler(resource_asyncload_handler);
	}
}
