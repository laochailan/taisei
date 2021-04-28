/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#ifndef IGUARD_stages_stage4_spells_spells_h
#define IGUARD_stages_stage4_spells_spells_h

#include "taisei.h"

#include "boss.h"

DECLARE_EXTERN_TASK_WITH_INTERFACE(kurumi_walachia, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(kurumi_dryfountain, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(kurumi_redspike, BossAttack);
void kurumi_aniwall(Boss*, int);
void kurumi_blowwall(Boss*, int);
void kurumi_danmaku(Boss*, int);
void kurumi_extra(Boss*, int);

#endif // IGUARD_stages_stage4_spells_spells_h
