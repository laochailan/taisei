/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#ifndef IGUARD_stages_stage3_wriggle_h
#define IGUARD_stages_stage3_wriggle_h

#include "taisei.h"

#include "boss.h"

DEFINE_ENTITY_TYPE(WriggleSlave, {
	struct {
		Sprite *circle, *particle;
	} sprites;

	cmplx pos;
	int spawn_time;
	Color color;
	cmplxf scale;

	COEVENTS_ARRAY(
		despawned
	) events;
});

Boss *stage3_spawn_wriggle(cmplx pos);
void stage3_draw_wriggle_spellbg(Boss *boss, int time);

void stage3_init_wriggle_slave(WriggleSlave *slave, cmplx pos);
WriggleSlave *stage3_host_wriggle_slave(cmplx pos);
void stage3_despawn_wriggle_slave(WriggleSlave *slave);

DECLARE_EXTERN_TASK(wriggle_slave_damage_trail, { BoxedWriggleSlave slave; });
DECLARE_EXTERN_TASK(wriggle_slave_follow, {
	BoxedWriggleSlave slave;
	BoxedBoss boss;
	real rot_speed;
	real rot_initial;
});

#endif // IGUARD_stages_stage3_wriggle_h
