/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "stage5.h"
#include "draw.h"
#include "background_anim.h"
#include "iku.h"
#include "spells/spells.h"
#include "timeline.h"

#include "global.h"
#include "portrait.h"
#include "common_tasks.h"

struct stage5_spells_s stage5_spells = {
	.mid = {
		.static_bomb = {
			{ 16, 17, 18, 19}, AT_SurvivalSpell, "High Voltage “Static Bomb”", 16, 10,
			NULL, NULL, BOSS_DEFAULT_GO_POS, 0,
			TASK_INDIRECT_INIT(BossAttack, stage5_midboss_static_bomb)
		},
	},

	.boss = {
		.atmospheric_discharge = {
			{ 0,  1,  2,  3}, AT_Spellcard, "High Voltage “Atmospheric Discharge”", 60, 44000,
			NULL, iku_spell_bg, BOSS_DEFAULT_GO_POS, 5,
			TASK_INDIRECT_INIT(BossAttack, stage5_spell_atmospheric_discharge)
		},
		.artificial_lightning = {
			{ 4,  5,  6,  7}, AT_Spellcard, "Charge Sign “Artificial Lightning”", 75, 60000,
			NULL, iku_spell_bg, BOSS_DEFAULT_GO_POS, 5,
			TASK_INDIRECT_INIT(BossAttack, stage5_spell_artificial_lightning)
		},
		.induction_field = {
			{12, 13, -1, -1}, AT_Spellcard, "Current Sign “Induction Field”", 60, 50000,
			NULL, iku_spell_bg, BOSS_DEFAULT_GO_POS, 5,
			TASK_INDIRECT_INIT(BossAttack, stage5_spell_induction)
		},
		.inductive_resonance = {
			{-1, -1, 14, 15}, AT_Spellcard, "Current Sign “Inductive Resonance”", 60, 50000,
			NULL, iku_spell_bg, BOSS_DEFAULT_GO_POS, 5,
			TASK_INDIRECT_INIT(BossAttack, stage5_spell_induction)
		},
		.natural_cathode = {
			{ 8,  9, 10, 11}, AT_Spellcard, "Spark Sign “Natural Cathode”", 60, 44000,
			NULL, iku_spell_bg, BOSS_DEFAULT_GO_POS, 5,
			TASK_INDIRECT_INIT(BossAttack, stage5_spell_natural_cathode)
		},
	},

	.extra.overload = {
		{ 0,  1,  2,  3}, AT_ExtraSpell, "Circuit Sign “Overload”", 60, 44000,
		iku_overload, iku_spell_bg, BOSS_DEFAULT_GO_POS, 5
	},
};

static void stage5_start(void) {
	stage5_drawsys_init();
	stage5_bg_init_fullstage();
	stage_start_bgm("stage5");
	stage_set_voltage_thresholds(255, 480, 860, 1250);
	INVOKE_TASK(stage5_timeline);
}

static void stage5_preload(void) {
	portrait_preload_base_sprite("iku", NULL, RESF_DEFAULT);
	portrait_preload_face_sprite("iku", "normal", RESF_DEFAULT);
	preload_resources(RES_BGM, RESF_OPTIONAL, "stage5", "stage5boss", NULL);
	preload_resources(RES_SPRITE, RESF_DEFAULT,
		"part/blast_huge_halo",
		"part/blast_huge_rays",
		"stage5/noise",
		"stage5/spell_bg",
		"stage5/spell_clouds",
		"stage5/spell_lightning",
		"stage5/tower",
	NULL);
	preload_resources(RES_SHADER_PROGRAM, RESF_DEFAULT,
		"tower_light",
	NULL);
	preload_resources(RES_SHADER_PROGRAM, RESF_OPTIONAL,
		"lasers/linear",
		"lasers/accelerated",
		"lasers/iku_cathode",
		"lasers/iku_lightning",
	NULL);
	preload_resources(RES_ANIM, RESF_DEFAULT,
		"boss/iku",
		"boss/iku_mid",
	NULL);
	preload_resources(RES_MODEL, RESF_DEFAULT,
		"tower",
	NULL);
	preload_resources(RES_SFX, RESF_OPTIONAL,
		"boom",
		"laser1",
		"enemydeath",
	NULL);
}

static void stage5_end(void) {
	stage5_drawsys_shutdown();
}

static void stage5_spellpractice_start(void) {
	stage5_drawsys_init();
	stage5_bg_init_spellpractice();

	global.boss = stage5_spawn_iku(BOSS_DEFAULT_SPAWN_POS);
	boss_add_attack_from_info(global.boss, global.stage->spell, true);
	boss_engage(global.boss);

	stage_start_bgm("stage5boss");
}

ShaderRule stage5_shaders[] = { NULL };

StageProcs stage5_procs = {
	.begin = stage5_start,
	.preload = stage5_preload,
	.end = stage5_end,
	.draw = stage5_draw,
	.shader_rules = stage5_shaders,
	.spellpractice_procs = &stage5_spell_procs,
};

StageProcs stage5_spell_procs = {
	.begin = stage5_spellpractice_start,
	.preload = stage5_preload,
	.end = stage5_end,
	.draw = stage5_draw,
	.shader_rules = stage5_shaders,
};
