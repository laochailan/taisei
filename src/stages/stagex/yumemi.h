/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#ifndef IGUARD_stages_stagex_yumemi_h
#define IGUARD_stages_stagex_yumemi_h

#include "taisei.h"

#include "entity.h"
#include "resource/sprite.h"
#include "coroutine.h"

DEFINE_ENTITY_TYPE(YumemiSlave, {
	struct {
		Sprite *core, *frame, *outer;
	} sprites;

	cmplx pos;
	int spawn_time;
	float alpha;
	float rotation_factor;
	float glitch_strength;

	COEVENTS_ARRAY(
		despawned
	) events;
});

void stagex_init_yumemi_slave(YumemiSlave *slave, cmplx pos, int type);
YumemiSlave *stagex_host_yumemi_slave(cmplx pos, int type);
void stagex_despawn_yumemi_slave(YumemiSlave *slave);

Boss *stagex_spawn_yumemi(cmplx pos);

void stagex_draw_yumemi_spellbg_voronoi(Boss *boss, int time);

#endif // IGUARD_stages_stagex_yumemi_h
