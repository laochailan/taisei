/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef LIST_H
#define LIST_H

#include <stdbool.h>

/* I got annoyed of the code doubling caused by simple linked lists,
 * so i do some void-magic here to save the lines.
 */

void *create_element(void **dest, int size);
void delete_element(void **dest, void *e);
void delete_all_elements(void **dest, void (callback)(void **, void *));
void delete_all_or_transient_elements(void **dest, void (callback)(void **, void *), bool transient);

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
#endif
