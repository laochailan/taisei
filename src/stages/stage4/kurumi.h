/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#ifndef IGUARD_stages_stage4_kurumi_h
#define IGUARD_stages_stage4_kurumi_h

#include "taisei.h"

#include "boss.h"

DECLARE_EXTERN_TASK(stage4_boss_nonspell_burst, { BoxedBoss boss; int duration; int count; });
DECLARE_EXTERN_TASK(stage4_boss_nonspell_redirect, { BoxedProjectile proj; MoveParams new_move; });

DECLARE_EXTERN_TASK(stage4_boss_slave_visual, { cmplx *pos; int interval; });

Boss *stage4_spawn_kurumi(cmplx pos);
void kurumi_slave_visual(Enemy *e, int t, bool render);
void kurumi_slave_static_visual(Enemy *e, int t, bool render);
void kurumi_spell_bg(Boss*, int);

#endif // IGUARD_stages_stage4_kurumi_h
