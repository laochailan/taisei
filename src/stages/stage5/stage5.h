/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_stages_stage5_stage5_h
#define IGUARD_stages_stage5_stage5_h

#include "taisei.h"

#include "stageinfo.h"

extern struct stage5_spells_s {
	// this struct must contain only fields of type AttackInfo
	// order of fields affects the visual spellstage number, but not its real internal ID

	struct {
		AttackInfo static_bomb;
	} mid;

	struct {
		AttackInfo atmospheric_discharge;
		AttackInfo artificial_lightning;
		AttackInfo induction_field;
		AttackInfo inductive_resonance;
		AttackInfo natural_cathode;
	} boss;

	struct {
		AttackInfo overload;
	} extra;

	// required for iteration
	AttackInfo null;
} stage5_spells;

extern StageProcs stage5_procs;
extern StageProcs stage5_spell_procs;

void stage5_skip(int);

#endif // IGUARD_stages_stage5_stage5_h
