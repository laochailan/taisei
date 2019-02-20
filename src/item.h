/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef IGUARD_item_h
#define IGUARD_item_h

#include "taisei.h"

#include "util.h"
#include "resource/texture.h"
#include "objectpool.h"
#include "entity.h"

typedef struct Item Item;
typedef LIST_ANCHOR(Item) ItemList;

typedef enum {
	// from least important to most important
	// this affects the draw order
	ITEM_PIV = 1,
	ITEM_POINTS,
	ITEM_POWER_MINI,
	ITEM_POWER,
	ITEM_SURGE,
	ITEM_VOLTAGE,
	ITEM_BOMB_FRAGMENT,
	ITEM_LIFE_FRAGMENT,
	ITEM_BOMB,
	ITEM_LIFE,

	ITEM_FIRST = ITEM_PIV,
	ITEM_LAST = ITEM_LIFE,
} ItemType;

struct Item {
	ENTITY_INTERFACE_NAMED(Item, ent);

	int birthtime;
	int collecttime;
	complex pos;
	complex pos0;

	int auto_collect;
	ItemType type;
	float pickup_value;

	complex v;
};

Item *create_item(complex pos, complex v, ItemType type);
void delete_item(Item *item);
void delete_items(void);

Item* create_clear_item(complex pos, uint clear_flags);

int collision_item(Item *p);
void process_items(void);

void spawn_item(complex pos, ItemType type);
void spawn_and_collect_item(complex pos, ItemType type, float collect_value);

// The varargs are: ItemType type1, int num1, ItemType type2, int num2, ..., NULL
// e.g.: Point 10, Power 3, LifeFrag 2, Bomb 1, NULL
// WARNING: if you pass a float or double as the amount, it will not work! You must explicitly cast it to an int.
void spawn_items(complex pos, ...) attr_sentinel;
void spawn_and_collect_items(complex pos, double collect_value, ...) attr_sentinel;

bool collect_item(Item *item, float value);
void collect_all_items(float value);

void items_preload(void);

#define POWER_VALUE 3
#define POWER_VALUE_MINI 1

#define ITEM_MAX_VALUE 1.0
#define ITEM_MIN_VALUE 0.1

#endif // IGUARD_item_h
