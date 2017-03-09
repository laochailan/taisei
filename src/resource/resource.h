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
#include "sfx.h"
#include "bgm.h"
#include "shader.h"
#include "font.h"
#include "model.h"
#include "hashtable.h"
#include "assert.h"
#include "paths/native.h"

typedef enum ResourceType {
	RES_TEXTURE,
	RES_ANIM,
	RES_SFX,
	RES_BGM,
	RES_SHADER,
	RES_MODEL,
	RES_NUMTYPES,
} ResourceType;

typedef enum ResourceFlags {
	RESF_REQUIRED = 1,
	RESF_OVERRIDE = 2,
	RESF_TRANSIENT = 4
} ResourceFlags;

// All paths are relative to the current working directory, which can assumed to be the resources directory,
// unless mentioned otherwise.

// Converts a path into an abstract resource name to be used as the hashtable key.
// This method is optional, the default strategy is to take the path minus the prefix and extension.
// The returned name must be free()'d.
typedef char* (*ResourceNameFunc)(const char *path);

// Converts an abstract resource name into a path from which a resource with that name could be loaded.
// The path may not actually exist or be usable. The load function (see below) shall deal with such cases.
// The returned path must be free()'d.
// May return NULL on failure, but does not have to.
typedef char* (*ResourceFindFunc)(const char *name);

// Tells whether the resource handler should attempt to load a file, specified by a path.
typedef bool (*ResourceCheckFunc)(const char *path);

// Tells whether the resource specified by a path is transient.
typedef bool (*ResourceTransientFunc)(const char *path);

// Loads a resource at path and returns a pointer to it.
// Must return NULL and not crash the program on failure.
typedef void* (*ResourceLoadFunc)(const char *path);

// Unloads a resource, freeing all allocated to it memory.
typedef void (*ResourceUnloadFunc)(void *res);

// ResourceTransientFunc callback: resource treated as transient if its path contains "stage".
bool istransient_filepath(const char *path);

// ResourceTransientFunc callback: resource treated as transient if its filename contains "stage".
bool istransient_filename(const char *path);

typedef struct ResourceHandler {
	ResourceType type;
	ResourceNameFunc name;
	ResourceFindFunc find;
	ResourceCheckFunc check;
	ResourceTransientFunc istransient;
	ResourceLoadFunc load;
	ResourceUnloadFunc unload;
	Hashtable *mapping;
	char subdir[32];
} ResourceHandler;

typedef struct Resource {
	ResourceType type;
	ResourceFlags flags;

	union {
		void *data;
		Texture *texture;
		Animation *animation;
		Sound *sound;
		Music *music;
		Shader *shader;
		Model *model;
	};
} Resource;

typedef struct Resources {
	ResourceHandler handlers[RES_NUMTYPES];
	FBO fbg[2];
	FBO fsec;
} Resources;

extern Resources resources;

void init_resources(void);
void load_resources(void);
void free_resources(ResourceFlags flags);

Resource* get_resource(ResourceType type, const char *name, ResourceFlags flags);
Resource* insert_resource(ResourceType type, const char *name, void *data, ResourceFlags flags, const char *source);

void resource_util_strip_ext(char *path);
char* resource_util_basename(const char *prefix, const char *path);
const char* resource_util_filename(const char *path);

void print_resource_hashtables(void);

#endif
