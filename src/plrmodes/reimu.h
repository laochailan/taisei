/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_plrmodes_reimu_h
#define IGUARD_plrmodes_reimu_h

#include "taisei.h"

#include "plrmodes.h"
#include "dialog/reimu.h"

extern PlayerCharacter character_reimu;
extern PlayerMode plrmode_reimu_a;
extern PlayerMode plrmode_reimu_b;

double reimu_common_property(Player *plr, PlrProperty prop);
int reimu_common_ofuda(Projectile *p, int t);
Projectile *reimu_common_ofuda_swawn_trail(Projectile *p);
void reimu_common_draw_yinyang(Enemy *e, int t, const Color *c);
void reimu_common_bomb_bg(Player *p, float alpha);
void reimu_common_bomb_buffer_init(void);

DECLARE_EXTERN_TASK(reimu_common_slave_expire, {
	BoxedPlayer player;
	BoxedEnemy slave;
	BoxedTask slave_main_task;
	int retract_time;
});

#endif // IGUARD_plrmodes_reimu_h
