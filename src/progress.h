/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_progress_h
#define IGUARD_progress_h

#include "taisei.h"

#include <SDL.h>

#include "cutscenes/cutscene.h"

#define PROGRESS_FILE "storage/progress.dat"
#define PROGRESS_MAXFILESIZE 4096

#ifdef DEBUG
	// #define PROGRESS_UNLOCK_ALL
#endif

typedef enum ProgfileCommand {
	// Do not reorder this!

	PCMD_UNLOCK_STAGES                     = 0x00,
	PCMD_UNLOCK_STAGES_WITH_DIFFICULTY     = 0x01,
	PCMD_HISCORE                           = 0x02,
	PCMD_STAGE_PLAYINFO                    = 0x03,
	PCMD_ENDINGS                           = 0x04,
	PCMD_GAME_SETTINGS                     = 0x05,
	PCMD_GAME_VERSION                      = 0x06,
	PCMD_UNLOCK_BGMS                       = 0x07,
	PCMD_UNLOCK_CUTSCENES                  = 0x08,
} ProgfileCommand;

typedef struct StageProgress {
	// Keep this struct small if you can
	// See stage_get_progress_from_info() in stage.c for more information

	uint32_t num_played;
	uint32_t num_cleared;
	uchar unlocked : 1;
} StageProgress;

typedef enum ProgressBGMID {
	// Do not reorder! Append at the end only, reserve space if removing.

	PBGM_MENU,
	PBGM_STAGE1,
	PBGM_STAGE1_BOSS,
	PBGM_STAGE2,
	PBGM_STAGE2_BOSS,
	PBGM_STAGE3,
	PBGM_STAGE3_BOSS,
	PBGM_STAGE4,
	PBGM_STAGE4_BOSS,
	PBGM_STAGE5,
	PBGM_STAGE5_BOSS,
	PBGM_STAGE6,
	PBGM_STAGE6_BOSS1,
	PBGM_STAGE6_BOSS2,
	PBGM_STAGE6_BOSS3,
	PBGM_ENDING,
	PBGM_CREDITS,
	PBGM_BONUS0,  // old Iku theme
	PBGM_BONUS1,  // Scuttle theme
	PBGM_GAMEOVER,
	PBGM_STAGEX,
	PBGM_STAGEX_BOSS,
} ProgressBGMID;

struct UnknownCmd;

typedef enum EndingID {
	// WARNING: Reordering this will break current progress files.

	ENDING_BAD_MARISA,
	ENDING_BAD_YOUMU,
	ENDING_GOOD_MARISA,
	ENDING_GOOD_YOUMU,
	ENDING_BAD_REIMU,
	ENDING_GOOD_REIMU,
	NUM_ENDINGS,
} EndingID;

#define GOOD_ENDINGS \
	ENDING(ENDING_GOOD_MARISA) \
	ENDING(ENDING_GOOD_YOUMU) \
	ENDING(ENDING_GOOD_REIMU)

#define BAD_ENDINGS \
	ENDING(ENDING_BAD_MARISA) \
	ENDING(ENDING_BAD_YOUMU) \
	ENDING(ENDING_BAD_REIMU)

typedef struct GlobalProgress {
	uint32_t hiscore;
	uint32_t achieved_endings[NUM_ENDINGS];
	uint64_t unlocked_bgms;
	uint64_t unlocked_cutscenes;
	struct UnknownCmd *unknown;

	struct {
		uint8_t difficulty;
		uint8_t character;
		uint8_t shotmode;
	} game_settings;
} GlobalProgress;



extern GlobalProgress progress;

void progress_load(void);
void progress_save(void);
void progress_unload(void);

uint32_t progress_times_any_ending_achieved(void);
uint32_t progress_times_any_good_ending_achieved(void);

bool progress_is_bgm_unlocked(const char *name);
void progress_unlock_bgm(const char *name);

void progress_track_ending(EndingID id);
bool progress_is_cutscene_unlocked(CutsceneID id);
void progress_unlock_cutscene(CutsceneID id);

#endif // IGUARD_progress_h
