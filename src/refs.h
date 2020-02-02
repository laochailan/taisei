/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_refs_h
#define IGUARD_refs_h

#include "taisei.h"

/*
 * NOTE: if you're here to attempt fixing any of this braindeath, better just delete the file and start over
 */

typedef struct {
	void *ptr;
	int refs;
} Reference;

typedef struct {
	Reference *ptrs;
	int count;
} RefArray;

extern void *_FREEREF;
#define FREEREF &_FREEREF
#define REF(p) (global.refs.ptrs[(int)(p)].ptr)
int add_ref(void *ptr);
void del_ref(void *ptr);
void free_ref(int i);
void free_all_refs(void);

#define UPDATE_REF(ref, ptr) ((ptr) = REF(ref))

#endif // IGUARD_refs_h
