/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "stageinfo.h"

#include "stages/stage1/stage1.h"
#include "stages/stage2/stage2.h"
#include "stages/stage3/stage3.h"
#include "stages/stage4/stage4.h"
#include "stages/stage5/stage5.h"
#include "stages/stage6/stage6.h"
#include "stages/stagex/stagex.h"

#ifdef DEBUG
	#define DPSTEST
	#include "stages/dpstest.h"

	#define COROTEST
	#include "stages/corotest.h"
#endif

static struct {
	DYNAMIC_ARRAY(StageInfo) stages;
} stageinfo;

static void add_stage(
	uint16_t id,
	StageProcs *procs,
	StageType type,
	const char *title,
	const char *subtitle,
	AttackInfo *spell,
	Difficulty diff
) {
	StageInfo *stg = dynarray_append(&stageinfo.stages);
	stg->id = id;
	stg->procs = procs;
	stg->type = type;
	stralloc(&stg->title, title);
	stralloc(&stg->subtitle, subtitle);
	stg->spell = spell;
	stg->difficulty = diff;
}

static void add_spellpractice_stage(
	StageInfo *s,
	AttackInfo *a,
	int *spellnum,
	uint16_t spellbits,
	Difficulty diff
) {
	uint16_t id = spellbits | a->idmap[diff - D_Easy] | (s->id << 8);

	char *title = strfmt("Spell %d", ++(*spellnum));
	char *subtitle = strjoin(a->name, " ~ ", difficulty_name(diff), NULL);

	add_stage(id, s->procs->spellpractice_procs, STAGE_SPELL, title, subtitle, a, diff);

	free(title);
	free(subtitle);
}

static void add_spellpractice_stages(
	int *spellnum,
	bool (*filter)(AttackInfo*),
	uint16_t spellbits
) {
	for(int i = 0 ;; ++i) {
		StageInfo *s = dynarray_get_ptr(&stageinfo.stages, i);

		if(s->type == STAGE_SPELL || !s->spell) {
			break;
		}

		for(AttackInfo *a = s->spell; a->name; ++a) {
			if(!filter(a)) {
				continue;
			}

			for(Difficulty diff = D_Easy; diff < D_Easy + NUM_SELECTABLE_DIFFICULTIES; ++diff) {
				if(a->idmap[diff - D_Easy] >= 0) {
					add_spellpractice_stage(s, a, spellnum, spellbits, diff);

					// stages may have gotten realloc'd, so we must update the pointer
					s = dynarray_get_ptr(&stageinfo.stages, i);
				}
			}
		}
	}
}

static bool spellfilter_normal(AttackInfo *spell) {
	return spell->type != AT_ExtraSpell;
}

static bool spellfilter_extra(AttackInfo *spell) {
	return spell->type == AT_ExtraSpell;
}

void stageinfo_init(void) {
	int spellnum = 0;

//	         id  procs          type         title      subtitle                       spells                       diff
	add_stage(1, &stage1_procs, STAGE_STORY, "Stage 1", "Misty Lake",                  (AttackInfo*)&stage1_spells, D_Any);
	add_stage(2, &stage2_procs, STAGE_STORY, "Stage 2", "Walk Along the Border",       (AttackInfo*)&stage2_spells, D_Any);
	add_stage(3, &stage3_procs, STAGE_STORY, "Stage 3", "Through the Tunnel of Light", (AttackInfo*)&stage3_spells, D_Any);
	add_stage(4, &stage4_procs, STAGE_STORY, "Stage 4", "Forgotten Mansion",           (AttackInfo*)&stage4_spells, D_Any);
	add_stage(5, &stage5_procs, STAGE_STORY, "Stage 5", "Climbing the Tower of Babel", (AttackInfo*)&stage5_spells, D_Any);
	add_stage(6, &stage6_procs, STAGE_STORY, "Stage 6", "Roof of the World",           (AttackInfo*)&stage6_spells, D_Any);

	add_stage(7, &stagex_procs, STAGE_EXTRA, "Extra Stage", "Descent into Madness",    (AttackInfo*)&stagex_spells, D_Extra);

#ifdef DPSTEST
	add_stage(0x40|0, &stage_dpstest_single_procs, STAGE_SPECIAL, "DPS Test", "Single target",    NULL, D_Normal);
	add_stage(0x40|1, &stage_dpstest_multi_procs,  STAGE_SPECIAL, "DPS Test", "Multiple targets", NULL, D_Normal);
	add_stage(0x40|2, &stage_dpstest_boss_procs,   STAGE_SPECIAL, "DPS Test", "Boss",             NULL, D_Normal);
#endif

	// generate spellpractice stages
	add_spellpractice_stages(&spellnum, spellfilter_normal, STAGE_SPELL_BIT);
	add_spellpractice_stages(&spellnum, spellfilter_extra, STAGE_SPELL_BIT | STAGE_EXTRASPELL_BIT);

#ifdef SPELL_BENCHMARK
	add_spellpractice_stage(dynarray_get_ptr(&stageinfo.stages, 0), &stage1_spell_benchmark, &spellnum, STAGE_SPELL_BIT, D_Extra);
#endif

#ifdef COROTEST
	add_stage(0xC0, &corotest_procs, STAGE_SPECIAL, "Coroutines!", "wow such concurrency very async", NULL, D_Any);
#endif

	dynarray_compact(&stageinfo.stages);

#ifdef DEBUG
	dynarray_foreach(&stageinfo.stages, int i, StageInfo *stg, {
		if(stg->type == STAGE_SPELL && !(stg->id & STAGE_SPELL_BIT)) {
			log_fatal("Spell stage has an ID without the spell bit set: 0x%04x", stg->id);
		}

		dynarray_foreach(&stageinfo.stages, int j, StageInfo *stg2, {
			if(stg != stg2 && stg->id == stg2->id) {
				log_fatal("Duplicate ID %X in stages array, indices: %i, %i", stg->id, i, j);
			}
		});
	});
#endif
}

void stageinfo_shutdown(void) {
	dynarray_foreach_elem(&stageinfo.stages, StageInfo *stg, {
		free(stg->title);
		free(stg->subtitle);
		free(stg->progress);
	});

	dynarray_free_data(&stageinfo.stages);
}

size_t stageinfo_get_num_stages(void) {
	return stageinfo.stages.num_elements;
}

StageInfo *stageinfo_get_by_index(size_t index) {
	return dynarray_get_ptr(&stageinfo.stages, index);
}

StageInfo *stageinfo_get_by_id(uint16_t n) {
	// TODO speed this up (use a hashtable maybe)

	dynarray_foreach_elem(&stageinfo.stages, StageInfo *stg, {
		if(stg->id == n) {
			return stg;
		}
	});

	return NULL;
}

StageInfo *stageinfo_get_by_spellcard(AttackInfo *spell, Difficulty diff) {
	if(!spell) {
		return NULL;
	}

	dynarray_foreach_elem(&stageinfo.stages, StageInfo *stg, {
		if(stg->spell == spell && stg->difficulty == diff) {
			return stg;
		}
	});

	return NULL;
}

StageProgress *stageinfo_get_progress(StageInfo *stage, Difficulty diff, bool allocate) {
	// D_Any stages will have a separate StageProgress for every selectable difficulty.
	// Stages with a fixed difficulty setting (spellpractice, extra stage...) obviously get just one and the diff parameter is ignored.

	// This stuff must stay around until progress_save(), which happens on shutdown.
	// So do NOT try to free any pointers this function returns, that will fuck everything up.

	bool fixed_diff = (stage->difficulty != D_Any);

	if(!fixed_diff && (diff < D_Easy || diff > D_Lunatic)) {
		return NULL;
	}

	if(!stage->progress) {
		if(!allocate) {
			return NULL;
		}

		size_t allocsize = sizeof(StageProgress) * (fixed_diff ? 1 : NUM_SELECTABLE_DIFFICULTIES);
		stage->progress = malloc(allocsize);
		memset(stage->progress, 0, allocsize);
	}

	return stage->progress + (fixed_diff ? 0 : diff - D_Easy);
}

StageProgress *stageinfo_get_progress_by_id(uint16_t id, Difficulty diff, bool allocate) {
	StageInfo *info = stageinfo_get_by_id(id);

	if(info != NULL) {
		return stageinfo_get_progress(info, diff, allocate);
	}

	return NULL;
}
