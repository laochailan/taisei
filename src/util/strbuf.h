/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#ifndef IGUARD_util_strbuf_h
#define IGUARD_util_strbuf_h

#include "taisei.h"

#include <stdarg.h>

typedef struct StringBuffer {
	char *start;
	char *pos;
	size_t buf_size;
} StringBuffer;

int strbuf_printf(StringBuffer *strbuf, const char *format, ...)
	attr_printf(2, 3) attr_nonnull_all;

int strbuf_vprintf(StringBuffer *strbuf, const char *format, va_list args)
	attr_nonnull_all;

void strbuf_clear(StringBuffer *strbuf)
	attr_nonnull_all;

void strbuf_free(StringBuffer *strbuf)
	attr_nonnull_all;

#endif // IGUARD_util_strbuf_h
