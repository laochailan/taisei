/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "plrmodes.h"
#include "youmu_a.h"
#include "youmu_b.h"

extern PlayerCharacter character_youmu;

void youmu_common_shot(Player *plr);
void youmu_common_draw_proj(Projectile *p, Color c, float scale);
void youmu_common_bombbg(Player *plr);
