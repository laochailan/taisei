/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_util_crap_h
#define IGUARD_util_crap_h

#include "taisei.h"

#include <SDL.h>

void* memdup(const void *src, size_t size) attr_returns_allocated attr_nonnull(1);
void inherit_missing_pointers(uint num, void *dest[num], void *const base[num]) attr_nonnull(2, 3);
bool is_main_thread(void);

typedef union FloatBits {
	float val;
	uint32_t bits;
} FloatBits;

typedef union DoubleBits {
	double val;
	uint64_t bits;
} DoubleBits;

INLINE uint32_t float_to_bits(float f) {
	return ((FloatBits) { .val = f }).bits;
}

INLINE float bits_to_float(uint32_t i) {
	return ((FloatBits) { .bits = i }).val;
}

INLINE uint64_t double_to_bits(double d) {
	return ((DoubleBits) { .val = d }).bits;
}

INLINE double bits_to_double(uint64_t i) {
	return ((DoubleBits) { .bits = i }).val;
}

extern SDL_threadID main_thread_id;

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(*(arr)))

#endif // IGUARD_util_crap_h
