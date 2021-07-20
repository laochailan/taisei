/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_resource_resource_h
#define IGUARD_resource_resource_h

#include "taisei.h"

#include "hashtable.h"

typedef enum ResourceType {
	RES_TEXTURE,
	RES_ANIM,
	RES_SFX,
	RES_BGM,
	RES_SHADER_OBJECT,
	RES_SHADER_PROGRAM,
	RES_MODEL,
	RES_POSTPROCESS,
	RES_SPRITE,
	RES_FONT,
	RES_MATERIAL,
	RES_NUMTYPES,
} ResourceType;

typedef enum ResourceFlags {
	RESF_OPTIONAL = 1,
	RESF_PERMANENT = 2,
	RESF_PRELOAD = 4,

	RESF_DEFAULT = 0,
} ResourceFlags;

typedef struct ResourceLoadState ResourceLoadState;

struct ResourceLoadState {
	const char *name;
	const char *path;
	void *opaque;
	ResourceFlags flags;
};

// Converts an abstract resource name into a vfs path from which a resource with that name could be loaded.
// The path may not actually exist or be usable. The load function (see below) shall deal with such cases.
// The returned path must be free()'d.
// May return NULL on failure, but does not have to.
typedef char* (*ResourceFindProc)(const char *name);

// Tells whether the resource handler should attempt to load a file, specified by a vfs path.
typedef bool (*ResourceCheckProc)(const char *path);

// Begins loading a resource. Called on an unspecified thread.
// Must call one of the following res_load_* functions before returning to indicate status.
typedef void (*ResourceLoadProc)(ResourceLoadState *st);

void res_load_failed(ResourceLoadState *st) attr_nonnull(1);
void res_load_finished(ResourceLoadState *st, void *res) attr_nonnull(1, 2);

// Indicates that loading must continue on the main thread.
// Schedules callback to be called on the main thread at some point in the future.
// The callback's load state parameter will contain the opaque pointer passed to this function.
// If the resource has dependencies, callback will not be called until they finish loading.
void res_load_continue_on_main(ResourceLoadState *st, ResourceLoadProc callback, void *opaque) attr_nonnull(1, 2);

// Like res_load_continue_on_main, but may be called from a worker thread.
// Use this to wait for dependencies if nothing needs to be done on the main thread.
void res_load_continue_after_dependencies(ResourceLoadState *st, ResourceLoadProc callback, void *opaque) attr_nonnull(1, 2);

// Registers another resource as a dependency and schedules it for asynchronous loading.
// Use with res_load_continue_after_dependencies/res_load_continue_on_main to wait for dependencies.
void res_load_dependency(ResourceLoadState *st, ResourceType type, const char *name);

// Unloads a resource, freeing all allocated to it memory.
typedef void (*ResourceUnloadProc)(void *res);

// Called during resource subsystem initialization
typedef void (*ResourceInitProc)(void);

// Called after the resources and rendering subsystems have been initialized
typedef void (*ResourcePostInitProc)(void);

// Called during resource subsystem shutdown
typedef void (*ResourceShutdownProc)(void);

typedef struct ResourceHandler {
	ResourceType type;

	char *typename;
	char *subdir;

	struct {
		ResourceFindProc find;
		ResourceCheckProc check;
		ResourceLoadProc load;
		ResourceUnloadProc unload;
		ResourceInitProc init;
		ResourcePostInitProc post_init;
		ResourceShutdownProc shutdown;
	} procs;

	struct {
		ht_str2ptr_ts_t mapping;
	} private;
} ResourceHandler;

typedef struct Resource {
	void *data;
	ResourceType type;
	ResourceFlags flags;
} Resource;

void init_resources(void);
void load_resources(void);
void free_resources(bool all);

Resource *_get_resource(ResourceType type, const char *name, hash_t hash, ResourceFlags flags) attr_nonnull_all;
void *_get_resource_data(ResourceType type, const char *name, hash_t hash, ResourceFlags flags) attr_nonnull_all;

attr_nonnull_all
INLINE Resource *get_resource(ResourceType type, const char *name, ResourceFlags flags) {
	return _get_resource(type, name, ht_str2ptr_hash(name), flags);
}

attr_nonnull_all
INLINE void *get_resource_data(ResourceType type, const char *name, ResourceFlags flags) {
	return _get_resource_data(type, name, ht_str2ptr_hash(name), flags);
}

void preload_resource(ResourceType type, const char *name, ResourceFlags flags);
void preload_resources(ResourceType type, ResourceFlags flags, const char *firstname, ...) attr_sentinel;
void *resource_for_each(ResourceType type, void *(*callback)(const char *name, Resource *res, void *arg), void *arg);

void resource_util_strip_ext(char *path);
char *resource_util_basename(const char *prefix, const char *path);
const char *resource_util_filename(const char *path);

#define DEFINE_RESOURCE_GETTER(_type, _name, _enum) \
	attr_nonnull_all attr_returns_nonnull \
	INLINE _type *_name(const char *resname) { \
		return NOT_NULL(get_resource_data(_enum, resname, RESF_DEFAULT)); \
	}

#define DEFINE_OPTIONAL_RESOURCE_GETTER(_type, _name, _enum) \
	attr_nonnull_all \
	INLINE _type *_name(const char *resname) { \
		return get_resource_data(_enum, resname, RESF_OPTIONAL); \
	}

#define DEFINE_DEPRECATED_RESOURCE_GETTER(_type, _name, _successor) \
	attr_deprecated("Use " #_successor "() instead") \
	INLINE _type *_name(const char *resname) { \
		return _successor(resname); \
	}

#endif // IGUARD_resource_resource_h
