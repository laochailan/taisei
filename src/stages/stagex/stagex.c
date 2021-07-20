/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "stagex.h"
#include "yumemi.h"
#include "draw.h"
#include "background_anim.h"
#include "timeline.h"

#include "coroutine.h"
#include "global.h"

/*
 *  See the definition of AttackInfo in boss.h for information on how to set up the idmaps.
 *  To add, remove, or reorder spells, see this stage's header file.
 */

struct stagex_spells_s stagex_spells = {
	.midboss = {
		.trap_representation = {
			{-1, -1, -1, 3}, AT_Spellcard, "6.2.6.1p5 “Trap Representation”", 60, 20000, NULL, stagex_draw_yumemi_spellbg_voronoi, CMPLX(VIEWPORT_W/2,VIEWPORT_H/2), 7, TASK_INDIRECT_INIT(BossAttack, stagex_spell_trap_representation)
		},
		.fork_bomb = {
			{-1, -1, -1, 4}, AT_Spellcard, "IEEE 1003.1-1988 “fork()”", 60, 20000, NULL, stagex_draw_yumemi_spellbg_voronoi, CMPLX(VIEWPORT_W/2,VIEWPORT_H/2), 7, TASK_INDIRECT_INIT(BossAttack, stagex_spell_fork_bomb)
		},
	},
	.boss = {
		.infinity_network = {
			{-1, -1, -1, 1}, AT_SurvivalSpell, "Obliteration “Infinity Network”", 90, 80000,
			NULL, stagex_draw_yumemi_spellbg_voronoi, VIEWPORT_W/2.0+100.0*I, 7, TASK_INDIRECT_INIT(BossAttack, stagex_spell_infinity_network)
		},
		.sierpinski = {
			{-1, -1, -1, 2}, AT_Spellcard, "Automaton “Legacy of Sierpiński”", 90, 150000,
			NULL, stagex_draw_yumemi_spellbg_voronoi, VIEWPORT_W/2.0+120.0*I, 7, TASK_INDIRECT_INIT(BossAttack, stagex_spell_sierpinski)
		},
		.mem_copy = {
			{-1, -1, -1, 5}, AT_Spellcard, "Memory “Block-wise Copy”", 90, 150000,
			NULL, stagex_draw_yumemi_spellbg_voronoi, VIEWPORT_W/2.0+120.0*I, 7, TASK_INDIRECT_INIT(BossAttack, stagex_spell_mem_copy)
		},
		.pipe_dream = {
			{-1, -1, -1, 6}, AT_Spellcard, "Philosophy “Pipe Dream”", 90, 150000,
			NULL, stagex_draw_yumemi_spellbg_voronoi, VIEWPORT_W/2.0+120.0*I, 7, TASK_INDIRECT_INIT(BossAttack, stagex_spell_pipe_dream)
		},
	},
};

static void stagex_begin(void) {
	stagex_drawsys_init();
	stagex_bg_init_fullstage();

// 	INVOKE_TASK(stagex_timeline);
}

static void stagex_spellpractice_begin(void) {
	stagex_drawsys_init();

	StageXDrawData *draw_data = stagex_get_draw_data();
	draw_data->tower_global_dissolution = 1;
	draw_data->tower_partial_dissolution = 1;

	Boss *boss = stagex_spawn_yumemi(BOSS_DEFAULT_SPAWN_POS);
	boss_add_attack_from_info(boss, global.stage->spell, true);
	boss_engage(boss);
	global.boss = boss;

	stage_start_bgm("stagexboss");
}

static void stagex_end(void) {
	stagex_drawsys_shutdown();
}

static void stagex_preload(void) {
	preload_resources(RES_TEXTURE, RESF_DEFAULT,
		"cell_noise",
		"stage5/metal_diffuse",
		"stage5/metal_normal",
		"stage5/metal_roughness",
		"stage5/stairs_ambient",
		"stage5/stairs_diffuse",
		"stage5/stairs_normal",
		"stage5/stairs_roughness",
		"stage5/wall_ambient",
		"stage5/wall_diffuse",
		"stage5/wall_normal",
		"stage5/wall_roughness",
		"stagex/bg",
		"stagex/bg_binary",
		"stagex/code",
		"stagex/dissolve_mask",
	NULL);
	preload_resources(RES_SHADER_PROGRAM, RESF_DEFAULT,
		"copy_depth",
		"extra_bg",
		"extra_tower_apply_mask",
		"extra_tower_mask",
		"zbuf_fog",
	NULL);
	preload_resources(RES_MODEL, RESF_DEFAULT,
		"stage5/metal",
		"stage5/stairs",
		"stage5/wall",
		"tower_alt_uv",
	NULL);
}

StageProcs stagex_procs = {
	.begin = stagex_begin,
	.end = stagex_end,
	.draw = stagex_draw_background,
	.preload = stagex_preload,
	.shader_rules = stagex_bg_effects,
	.postprocess_rules = stagex_postprocess_effects,
	.spellpractice_procs = &stagex_spell_procs,
};

StageProcs stagex_spell_procs = {
	.begin = stagex_spellpractice_begin,
	.end = stagex_end,
	.draw = stagex_draw_background,
	.preload = stagex_preload,
	.shader_rules = stagex_bg_effects,
	.postprocess_rules = stagex_postprocess_effects,
};
