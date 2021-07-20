/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_stages_stagex_stagex_h
#define IGUARD_stages_stagex_stagex_h

#include "taisei.h"

#include "stageinfo.h"

extern struct stagex_spells_s {
	// this struct must contain only fields of type AttackInfo
	// order of fields affects the visual spellstage number, but not its real internal ID

	struct {
		AttackInfo trap_representation;
		AttackInfo fork_bomb;
	} midboss;

	struct {
		AttackInfo infinity_network;
		AttackInfo sierpinski;
		AttackInfo mem_copy;
		AttackInfo pipe_dream;
	} boss;

	// required for iteration
	AttackInfo null;
} stagex_spells;

extern StageProcs stagex_procs;
extern StageProcs stagex_spell_procs;

Boss *stagex_spawn_yumemi(cmplx pos);
void stagex_draw_yumemi_portrait_overlay(SpriteParams *sp);

DECLARE_EXTERN_TASK_WITH_INTERFACE(stagex_spell_trap_representation, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stagex_spell_fork_bomb, BossAttack);

DECLARE_EXTERN_TASK_WITH_INTERFACE(stagex_spell_infinity_network, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stagex_spell_sierpinski, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stagex_spell_mem_copy, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stagex_spell_pipe_dream, BossAttack);

#endif // IGUARD_stages_stagex_stagex_h
