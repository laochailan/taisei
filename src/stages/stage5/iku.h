/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_stages_stage5_iku_h
#define IGUARD_stages_stage5_iku_h

#include "taisei.h"

#include "boss.h"

DEFINE_ENTITY_TYPE(IkuSlave, {
	struct {
		Sprite *lightning0, *lightning1, *cloud;
	} sprites;

	cmplx pos;
	int spawn_time;
	Color color;
	cmplxf scale;

	COEVENTS_ARRAY(
		despawned
	) events;
});

DECLARE_EXTERN_TASK(iku_slave_move, {
	BoxedIkuSlave slave;
	MoveParams move;
});

void stage5_init_iku_slave(IkuSlave *slave, cmplx pos);
void iku_nonspell_spawn_cloud(void);
void iku_lightning_particle(cmplx);

Boss *stage5_spawn_iku(cmplx pos);
IkuSlave *stage5_midboss_slave(cmplx pos);

int iku_induction_bullet(Projectile*, int);

#endif // IGUARD_stages_stage5_iku_h
