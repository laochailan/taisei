/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#ifndef IGUARD_stages_stage5_spells_spells_h
#define IGUARD_stages_stage5_spells_spells_h

#include "taisei.h"

#include "common_tasks.h"

#include "stages/stage5/iku.h"

void iku_atmospheric(Boss *, int);
void iku_lightning(Boss *, int);
void iku_induction(Boss *, int);
void iku_cathode(Boss *, int);
void iku_overload(Boss *, int);


void iku_mid_intro(Boss *, int t);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage5_midboss_iku_explosion, BossAttack);

#endif // IGUARD_stages_stage5_spells_spells_h
