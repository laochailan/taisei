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

void stage5_init_iku_slave(IkuSlave *slave, cmplx pos);
IkuSlave *stage5_midboss_slave(cmplx pos);

void iku_slave_visual(Enemy*, int, bool);
void iku_nonspell_spawn_cloud(void);
void iku_lightning_particle(cmplx, int);

int iku_induction_bullet(Projectile*, int);

Boss *stage5_spawn_iku(cmplx pos);

#endif // IGUARD_stages_stage5_iku_h
