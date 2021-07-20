/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_util_kvparser_h
#define IGUARD_util_kvparser_h

#include "taisei.h"

#include <SDL.h>

#include "hashtable.h"

// The callback should return true on success and false on error. In the case of
// an error it is responsible for calling log_warn with a descriptive error
// message.
typedef bool (*KVCallback)(const char *key, const char *value, void *data);

typedef struct KVSpec {
	const char *name;

	char **out_str;
	int *out_int;
	long *out_long;
	double *out_double;
	float *out_float;
	bool *out_bool;

	uint min_values;
	uint max_values;

	KVCallback callback;
	void *callback_data;
} KVSpec;

bool parse_keyvalue_stream_cb(SDL_RWops *strm, KVCallback callback, void *data);
bool parse_keyvalue_file_cb(const char *filename, KVCallback callback, void *data);
bool parse_keyvalue_stream(SDL_RWops *strm, ht_str2ptr_t *hashtable);
bool parse_keyvalue_file(const char *filename, ht_str2ptr_t *hashtable);
bool parse_keyvalue_stream_with_spec(SDL_RWops *strm, KVSpec *spec);
bool parse_keyvalue_file_with_spec(const char *filename, KVSpec *spec);

bool parse_bool(const char *str, bool fallback) attr_nonnull(1);

bool kvparser_deprecation(const char *key, const char *val, void *data);
bool kvparser_vec3(const char *key, const char *val, void *data);

#define KVSPEC_DEPRECATED(new) .callback = kvparser_deprecation, .callback_data = (new)

#endif // IGUARD_util_kvparser_h
