/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "stage4.h"
#include "draw.h"
#include "background_anim.h"
#include "timeline.h"
#include "spells/spells.h"
#include "kurumi.h"

#include "portrait.h"
#include "stageutils.h"
#include "global.h"

/*
 *  See the definition of AttackInfo in boss.h for information on how to set up the idmaps.
 *  To add, remove, or reorder spells, see this stage's header file.
 */

struct stage4_spells_s stage4_spells = {
	.mid = {
		.gate_of_walachia = {
			{ 0,  1,  2,  3}, AT_Spellcard, "Bloodless “Gate of Walachia”", 50, 44000,
			kurumi_slaveburst, kurumi_spell_bg, BOSS_DEFAULT_GO_POS, 4
		},
		.dry_fountain = {
			{ 4,  5, -1, -1}, AT_Spellcard, "Bloodless “Dry Fountain”", 50, 44000,
			kurumi_redspike, kurumi_spell_bg, BOSS_DEFAULT_GO_POS, 4
		},
		.red_spike = {
			{-1, -1,  6,  7}, AT_Spellcard, "Bloodless “Red Spike”", 50, 46000,
			kurumi_redspike, kurumi_spell_bg, BOSS_DEFAULT_GO_POS, 4
		},
	},

	.boss = {
		.animate_wall = {
			{ 8,  9, -1, -1}, AT_Spellcard, "Limit “Animate Wall”", 60, 50000,
			kurumi_aniwall, kurumi_spell_bg, BOSS_DEFAULT_GO_POS, 4
		},
		.demon_wall = {
			{-1, -1, 10, 11}, AT_Spellcard, "Summoning “Demon Wall”", 60, 55000,
			kurumi_aniwall, kurumi_spell_bg, BOSS_DEFAULT_GO_POS, 4
		},
		.blow_the_walls = {
			{12, 13, 14, 15}, AT_Spellcard, "Power Sign “Blow the Walls”", 60, 55000,
			kurumi_blowwall, kurumi_spell_bg, BOSS_DEFAULT_GO_POS, 4
		},
		.bloody_danmaku = {
			{18, 19, 16, 17}, AT_Spellcard, "Predation “Vampiric Vapor”", 80, 60000,
			kurumi_danmaku, kurumi_spell_bg, BOSS_DEFAULT_GO_POS, 4
		},
	},

	.extra.vlads_army = {
		{ 0,  1,  2,  3}, AT_ExtraSpell, "Blood Magic “Vlad’s Army”", 60, 50000,
		kurumi_extra, kurumi_spell_bg, BOSS_DEFAULT_GO_POS, 4
	},
};

static void stage4_start(void) {
	stage4_drawsys_init();
	stage4_bg_init_fullstage();
	stage_start_bgm("stage4");
}

static void stage4_spellpractice_start(void) {
	stage4_drawsys_init();
	stage4_bg_init_spellpractice();

	global.boss = stage4_spawn_kurumi(BOSS_DEFAULT_SPAWN_POS);
	boss_add_attack_from_info(global.boss, global.stage->spell, true);
	boss_engage(global.boss);

	stage_start_bgm("stage4boss");
}

static void stage4_preload(void) {
	portrait_preload_base_sprite("kurumi", NULL, RESF_DEFAULT);
	portrait_preload_face_sprite("kurumi", "normal", RESF_DEFAULT);
	preload_resources(RES_BGM, RESF_OPTIONAL, "stage4", "stage4boss", NULL);
	preload_resources(RES_TEXTURE, RESF_DEFAULT,
		"stage4/kurumibg1",
		"stage4/kurumibg2",
	NULL);
	preload_resources(RES_SPRITE, RESF_DEFAULT,
		"stage6/scythe", // Stage 6 is intentional
	NULL);
	preload_resources(RES_SHADER_PROGRAM, RESF_DEFAULT,
		"alpha_discard",
		"pbr",
		"sprite_negative",
		"ssr_water",
		"zbuf_fog",
	NULL);
	preload_resources(RES_SHADER_PROGRAM, RESF_OPTIONAL,
		"lasers/accelerated",
	NULL);
	preload_resources(RES_ANIM, RESF_DEFAULT,
		"boss/kurumi",
	NULL);
	preload_resources(RES_MATERIAL, RESF_DEFAULT,
		"stage4/corridor",
		"stage4/ground",
		"stage4/mansion",
	NULL);
	preload_resources(RES_MODEL, RESF_DEFAULT,
		"stage4/corridor",
		"stage4/ground",
		"stage4/mansion",
	NULL);
	preload_resources(RES_TEXTURE, RESF_OPTIONAL,
		"part/sinewave",
	NULL);
	preload_resources(RES_SFX, RESF_OPTIONAL,
		"laser1",
		"boom",
		"warp",
	NULL);

	// XXX: Special case for spell practice of the god damn extra spell, because it always needs a special case.
	// TODO: Maybe add spell-specific preloads instead of putting everything into the stage one?
	enemies_preload();
}

static void stage4_end(void) {
	stage4_drawsys_shutdown();
}

StageProcs stage4_procs = {
	.begin = stage4_start,
	.preload = stage4_preload,
	.end = stage4_end,
	.draw = stage4_draw,
	.event = stage4_events,
	.shader_rules = stage4_bg_effects,
	.spellpractice_procs = &stage4_spell_procs,
};

StageProcs stage4_spell_procs = {
	.begin = stage4_spellpractice_start,
	.preload = stage4_preload,
	.end = stage4_end,
	.draw = stage4_draw,
	.shader_rules = stage4_bg_effects,
};
