/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "kvparser.h"
#include "log.h"
#include "stringops.h"
#include "io.h"
#include "vfs/public.h"

bool parse_keyvalue_stream_cb(SDL_RWops *strm, KVCallback callback, void *data) {
	static const char separator[] = "= ";

	size_t bufsize = 256;
	char *buffer = malloc(bufsize);
	int lineno = 0;
	int errors = 0;

	loopstart: while(SDL_RWgets_realloc(strm, &buffer, &bufsize)) {
		char *ptr = buffer;
		char *sep, *key, *val;

		++lineno;

		while(isspace(*ptr)) {
			if(!*(++ptr)) {
				// blank line
				goto loopstart;
			}
		}

		if(*ptr == '#') {
			// comment
			continue;
		}

		sep = strstr(ptr, separator);

		if(!sep) {
			++errors;
			log_warn("Syntax error on line %i: missing separator", lineno);
			continue;
		}

		// split it up
		*sep = 0;
		key = ptr;
		val = sep + sizeof(separator) - 1;

		// the separator may be preceeded by any kind of whitespace, so strip it from the key
		while(isspace(*(ptr = strchr(key, 0) - 1))) {
			*ptr = 0;
		}

		// strip any kind of line endings from the value
		while(strchr("\r\n", *(ptr = strchr(val, 0) - 1))) {
			*ptr = 0;
		}

		if(!callback(key, val, data)) {
			++errors;
			continue;
		}
	}

	free(buffer);
	return !errors;
}

bool parse_keyvalue_file_cb(const char *filename, KVCallback callback, void *data) {
	SDL_RWops *strm = vfs_open(filename, VFS_MODE_READ);

	if(!strm) {
		log_error("VFS error: %s", vfs_get_error());
		return false;
	}

	bool status = parse_keyvalue_stream_cb(strm, callback, data);
	SDL_RWclose(strm);
	return status;
}

static bool kvcallback_hashtable(const char *key, const char *val, void *data) {
	ht_str2ptr_t *ht = data;
	ht_set(ht, key, strdup(val));
	return true;
}

bool parse_keyvalue_stream(SDL_RWops *strm, ht_str2ptr_t *ht) {
	return parse_keyvalue_stream_cb(strm, kvcallback_hashtable, ht);
}

bool parse_keyvalue_file(const char *filename, ht_str2ptr_t  *ht) {
	return parse_keyvalue_file_cb(filename, kvcallback_hashtable, ht);
}

static bool kvcallback_spec(const char *key, const char *val, void *data) {
	KVSpec *spec = data;

	for(KVSpec *s = spec; s->name; ++s) {
		if(!strcmp(key, s->name)) {
			if(s->out_str) {
				stralloc(s->out_str, val);
			}

			if(s->out_int) {
				*s->out_int = strtol(val, NULL, 0);
			}

			if(s->out_long) {
				*s->out_long = strtol(val, NULL, 0);
			}

			if(s->out_float) {
				*s->out_float = strtod(val, NULL);
			}

			if(s->out_double) {
				*s->out_double = strtod(val, NULL);
			}

			if(s->out_bool) {
				*s->out_bool = parse_bool(val, *s->out_bool);
			}

			if(s->callback) {
				return s->callback(key, val, s->callback_data);
			}

			return true;
		}
	}

	log_warn("Unknown key '%s' with value '%s' ignored", key, val);
	return true;
}

bool parse_keyvalue_stream_with_spec(SDL_RWops *strm, KVSpec *spec) {
	return parse_keyvalue_stream_cb(strm, kvcallback_spec, spec);
}

bool parse_keyvalue_file_with_spec(const char *filename, KVSpec *spec) {
	return parse_keyvalue_file_cb(filename, kvcallback_spec, spec);
}

bool parse_bool(const char *str, bool fallback) {
	while(isspace(*str)) {
		++str;
	}

	char buf[strlen(str) + 1], *bufp = buf;

	while(*str && !isspace(*str)) {
		*bufp++ = *str++;
	}

	*bufp = 0;
	double numeric_val = strtod(buf, &bufp);

	if(*buf && !*bufp) {
		return (bool)numeric_val;
	}

	static const char *true_vals[]  = { "on",  "yes", "true",  NULL };
	static const char *false_vals[] = { "off", "no",  "false", NULL };

	for(const char **v = true_vals; *v; ++v) {
		if(!SDL_strcasecmp(buf, *v)) {
			return true;
		}
	}

	for(const char **v = false_vals; *v; ++v) {
		if(!SDL_strcasecmp(buf, *v)) {
			return false;
		}
	}

	log_warn("Bad value `%s`, assuming %s", buf, fallback ? "true" : "false");
	return fallback;
}

bool kvparser_deprecation(const char *key, const char *val, void *data) {
	const char *alt = data;
	log_warn("'%s' is deprecated, use '%s' instead", key, alt);
	return true;
}

bool kvparser_vec3(const char *key, const char *val, void *data) {
	float *out = data;
	char *p = (char*)val;

	out[0] = strtof(p, &p);
	if(!isspace(*p)) goto fail;
	out[1] = strtof(p, &p);
	if(!isspace(*p)) goto fail;
	out[2] = strtof(p, &p);

	while(isspace(*p))
		++p;

	if(*p == 0) {
		return true;
	}

fail:
	log_error("Invalid vec3 value for '%s': %s", key, val);
	return false;
}
