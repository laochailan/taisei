/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_stages_stage3_nonspells_nonspells_h
#define IGUARD_stages_stage3_nonspells_nonspells_h

#include "taisei.h"

#include "boss.h"

DECLARE_EXTERN_TASK_WITH_INTERFACE(stage3_midboss_nonspell_1, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage3_boss_nonspell_1, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage3_boss_nonspell_2, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage3_boss_nonspell_3, BossAttack);

#endif // IGUARD_stages_stage3_nonspells_nonspells_h
