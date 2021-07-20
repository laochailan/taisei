/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "stage6.h"

#include "draw.h"
#include "background_anim.h"
#include "elly.h"
#include "spells/spells.h"
#include "timeline.h"

#include "portrait.h"
#include "common_tasks.h"

MODERNIZE_THIS_FILE_AND_REMOVE_ME

/*
 *	See the definition of AttackInfo in boss.h for information on how to set up the idmaps.
 *	To add, remove, or reorder spells, see this stage's header file.
 */

struct stage6_spells_s stage6_spells = {
	.scythe = {
		.occams_razor = {
			{ 0,  1,  2,  3}, AT_Spellcard, "Newton Sign “Occam’s Razor”", 50, 60000,
			elly_newton, elly_spellbg_classic, BOSS_DEFAULT_GO_POS, 6
		},
		.orbital_clockwork = {
			{24, 25, 26, 27}, AT_Spellcard, "Kepler Sign “Orbital Clockwork”", 50, 60000,
			elly_kepler, elly_spellbg_classic, BOSS_DEFAULT_GO_POS, 6
		},
		.wave_theory = {
			{ 4,  5,  6,  7}, AT_Spellcard, "Maxwell Sign “Wave Theory”", 30, 25000,
			elly_maxwell, elly_spellbg_classic, BOSS_DEFAULT_GO_POS, 6
		},
	},

	.baryon = {
		.many_world_interpretation = {
			{ 8,  9, 10, 11}, AT_Spellcard, "Eigenstate “Many-World Interpretation”", 50, 60000,
			elly_eigenstate, elly_spellbg_modern, BOSS_DEFAULT_GO_POS, 7
		},
		.wave_particle_duality = {
			{28, 29, 30, 31}, AT_Spellcard, "de Broglie Sign “Wave-Particle Duality”", 60, 65000,
			elly_broglie, elly_spellbg_modern_dark, BOSS_DEFAULT_GO_POS, 7
		},
		.spacetime_curvature = {
			{12, 13, 14, 15}, AT_Spellcard, "Ricci Sign “Spacetime Curvature”", 50, 80000,
			elly_ricci, elly_spellbg_modern, BOSS_DEFAULT_GO_POS, 7
		},
		.higgs_boson_uncovered = {
			{16, 17, 18, 19}, AT_Spellcard, "LHC “Higgs Boson Uncovered”", 75, 60000,
			elly_lhc, elly_spellbg_modern, BOSS_DEFAULT_GO_POS, 7
		}
	},

	.extra = {
		.curvature_domination = {
			{ 0,  1,  2,  3}, AT_ExtraSpell, "Forgotten Universe “Curvature Domination”", 60, 60000,
			elly_curvature, elly_spellbg_modern, BOSS_DEFAULT_GO_POS, 7
		}
	},

	.final = {
		.theory_of_everything = {
			{20, 21, 22, 23}, AT_SurvivalSpell, "Tower of Truth “Theory of Everything”", 70, 40000,
			elly_theory, elly_spellbg_toe, ELLY_TOE_TARGET_POS, 8
		}
	},
};

static void stage6_preload(void) {
	portrait_preload_base_sprite("elly", NULL, RESF_DEFAULT);
	portrait_preload_face_sprite("elly", "normal", RESF_DEFAULT);
	portrait_preload_base_sprite("elly", "beaten", RESF_DEFAULT);
	portrait_preload_face_sprite("elly", "shouting", RESF_DEFAULT);
	preload_resources(RES_BGM, RESF_OPTIONAL,
		"stage6",
		"stage6boss_phase1",
		"stage6boss_phase2",
		"stage6boss_phase3",
	NULL);
	preload_resources(RES_TEXTURE, RESF_DEFAULT,
		"stage6/sky",
		"stage6/stairs_ambient",
		"stage6/stairs_diffuse",
		"stage6/stairs_normal",
		"stage6/stairs_roughness",
		"stage6/tower_ambient",
		"stage6/tower_diffuse",
		"stage6/tower_normal",
		"stage6/tower_roughness",
		"stage6/rim_ambient",
		"stage6/rim_diffuse",
		"stage6/rim_normal",
		"stage6/rim_roughness",
		"stage6/tower_bottom_ambient",
		"stage6/tower_bottom_diffuse",
		"stage6/tower_bottom_normal",
		"stage6/tower_bottom_roughness",
		"stage6/spires_diffuse",
		"stage6/spires_normal",
		"stage6/spires_roughness",
	NULL);
	preload_resources(RES_SPRITE, RESF_DEFAULT,
		"part/blast_huge_halo",
		"part/blast_huge_rays",
		"part/myon",
		"part/stardust",
		"proj/apple",
		"stage6/baryon",
		"stage6/baryon_center",
		"stage6/baryon_connector",
		"stage6/scythe",
		"stage6/spellbg_chalk",
		"stage6/spellbg_classic",
		"stage6/spellbg_modern",
		"stage6/spellbg_toe",
		"stage6/toelagrangian/0",
		"stage6/toelagrangian/1",
		"stage6/toelagrangian/2",
		"stage6/toelagrangian/3",
		"stage6/toelagrangian/4",
	NULL);
	preload_resources(RES_SHADER_PROGRAM, RESF_DEFAULT,
		"baryon_feedback",
		"calabi-yau-quintic",
		"envmap_reflect",
		"pbr",
		"stage6_sky",
		"zbuf_fog",
	NULL);
	preload_resources(RES_SHADER_PROGRAM, RESF_OPTIONAL,
		"lasers/accelerated",
		"lasers/circle",
		"lasers/elly_toe_fermion",
		"lasers/elly_toe_gluon",
		"lasers/elly_toe_higgs",
		"lasers/elly_toe_photon",
		"lasers/linear",
		"lasers/maxwell",
		"lasers/sine",
	NULL);
	preload_resources(RES_ANIM, RESF_DEFAULT,
		"boss/elly",
	NULL);
	preload_resources(RES_MATERIAL, RESF_DEFAULT,
		"stage6/rim",
		"stage6/spires",
		"stage6/stairs",
		"stage6/tower",
		"stage6/tower_bottom",
	NULL);
	preload_resources(RES_MODEL, RESF_DEFAULT,
		"cube",
		"stage6/calabi-yau-quintic",
		"stage6/rim",
		"stage6/spires",
		"stage6/stairs",
		"stage6/top_plate",
		"stage6/tower",
		"stage6/tower_bottom",
	NULL);
	preload_resources(RES_SFX, RESF_DEFAULT | RESF_OPTIONAL,
		"warp",
		"noise1",
		"boom",
		"laser1",
	NULL);
}

static void stage6_start(void) {
	stage6_drawsys_init();
	stage6_bg_init_fullstage();
}

static void stage6_end(void) {
	stage6_drawsys_shutdown();
}

static void stage6_spellpractice_start(void) {
	stage6_start();
	stage6_bg_init_spellpractice();

	global.boss = stage6_spawn_elly(BOSS_DEFAULT_SPAWN_POS);
	AttackInfo *s = global.stage->spell;

	if(STG6_SPELL_NEEDS_SCYTHE(s)) {
		create_enemy3c(-32 + 32*I, ENEMY_IMMUNE, scythe_draw, scythe_reset, 0, 1+0.2*I, 1);
		stage_start_bgm("stage6boss_phase1");
	} else if(STG6_SPELL_NEEDS_BARYON(s)) {
		elly_spawn_baryons(global.boss->pos);
		stage_start_bgm("stage6boss_phase2");
	} else if(s == &stage6_spells.final.theory_of_everything) {
		stage6_bg_start_fall_over();
		stage_start_bgm("stage6boss_phase3");
	} else {
		stage_start_bgm("stage6boss_phase2");
	}

	boss_add_attack_from_info(global.boss, global.stage->spell, true);
	boss_engage(global.boss);
}

static void stage6_spellpractice_events(void) {
	if(!global.boss) {
		enemy_kill_all(&global.enemies);
	}
}

StageProcs stage6_procs = {
	.begin = stage6_start,
	.preload = stage6_preload,
	.end = stage6_end,
	.draw = stage6_draw,
	.event = stage6_events,
	.shader_rules = stage6_bg_effects,
	.spellpractice_procs = &stage6_spell_procs,
};

StageProcs stage6_spell_procs = {
	.begin = stage6_spellpractice_start,
	.preload = stage6_preload,
	.end = stage6_end,
	.draw = stage6_draw,
	.event = stage6_spellpractice_events,
	.shader_rules = stage6_bg_effects,
};
