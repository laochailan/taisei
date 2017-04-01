/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include <zlib.h>

#include "progress.h"
#include "paths/native.h"
#include "stage.h"

/*

	This module implements a persistent storage of a player's game progress, such as unlocked stages, high-scores etc.

	Basic outline of the progress file structure (little-endian encoding):
		[uint64 magic] [uint32 checksum] [array of commands]

	Where a command is:
		[uint8 command] [uint16 size] [array of {size} bytes, contents depend on {command}]

	This way we can easily extend the structure to store whatever we want, without breaking compatibility.
	This is forwards-compatible as well: any unrecognized command can be skipped when reading, since its size is known.

	The checksum is calculated on the whole array of commands (see progress_checksum() for implementation).
	If the checksum is incorrect, the whole progress file is deemed invalid and ignored.

	All commands are optional and the array may be empty. In that case, the checksum may be omitted as well.

	Currently implemented commands (see also the ProgfileCommand enum in progress.h):

		- PCMD_UNLOCK_STAGES:
			Unlocks one or more stages. Only works for stages with fixed difficulty.

		- PCMD_UNLOCK_STAGES_WITH_DIFFICULTY:
			Unlocks one or more stages, each on a specific difficulty

		- PCMD_HISCORE
			Sets the "Hi-Score" (highest score ever attained in one game session)
*/

/*

	Now in case you wonder why I decided to do it this way instead of stuffing everything in the config file, here are a couple of reasons:

		- The config module, as of the time of writting, is messy enough and probably needs refactoring.
		  I don't want to mix user preferences with things that are not supposed to be directly edited on top of that.

		- I don't want someone to accidentally lose progress after deleting the config file because they wanted to restore the default settings.

		- I want to discourage players from editing this file should they find it. This is why it's not textual and has a checksum.
		  Of course that doesn't actually prevent one from cheating it, but if you know how to do that, you might as well grab the game's source code and make it do whatever you please.
		  As long as this can stop a l33th4XxXxX0r1998 armed with notepad.exe or a hex editor, I'm happy.

*/

GlobalProgress progress;

static uint8_t progress_magic_bytes[] = {
	0x00, 0x67, 0x74, 0x66, 0x6f, 0xe3, 0x83, 0x84
};

static char* progress_getpath(void) {
	return strfmt("%s/%s", get_config_path(), PROGRESS_FILENAME);
}

static uint32_t progress_checksum(uint8_t *buf, size_t num) {
	return crc32(0xB16B00B5, buf, num);
}

typedef struct UnknownCmd {
	struct UnknownCmd *next;
	struct UnknownCmd *prev;
	uint8_t cmd;
	uint16_t size;
	uint8_t *data;
} UnknownCmd;

static void progress_read(SDL_RWops *file) {
	int64_t filesize = SDL_RWsize(file);

	if(filesize < 0) {
		log_warn("SDL_RWsize() failed: %s", SDL_GetError());
		return;
	}

	if(filesize > PROGRESS_MAXFILESIZE) {
		log_warn("Progress file is huge (%li bytes, %i max)", (long)filesize, PROGRESS_MAXFILESIZE);
		return;
	}

	for(int i = 0; i < sizeof(progress_magic_bytes); ++i) {
		if(SDL_ReadU8(file) != progress_magic_bytes[i]) {
			log_warn("Invalid header");
			return;
		}
	}

	if(filesize - SDL_RWtell(file) < 4) {
		return;
	}

	uint32_t checksum_fromfile;
	// no byteswapping here
	SDL_RWread(file, &checksum_fromfile, 4, 1);

	size_t bufsize = filesize - sizeof(progress_magic_bytes) - 4;
	uint8_t *buf = malloc(bufsize);

	if(!SDL_RWread(file, buf, bufsize, 1)) {
		log_warn("SDL_RWread() failed: %s", SDL_GetError());
		free(buf);
		return;
	}

	SDL_RWops *vfile = SDL_RWFromMem(buf, bufsize);
	uint32_t checksum = progress_checksum(buf, bufsize);

	if(checksum != checksum_fromfile) {
		log_warn("Bad checksum: %x != %x", checksum, checksum_fromfile);
		SDL_RWclose(vfile);
		free(buf);
		return;
	}

	while(SDL_RWtell(vfile) < bufsize) {
		ProgfileCommand cmd = (int8_t)SDL_ReadU8(vfile);
		uint16_t cur = 0;
		uint16_t cmdsize = SDL_ReadLE16(vfile);

		switch(cmd) {
			case PCMD_UNLOCK_STAGES:
				while(cur < cmdsize) {
					StageProgress *p = stage_get_progress(SDL_ReadLE16(vfile), D_Any, true);
					if(p) {
						p->unlocked = true;
					}
					cur += 2;
				}
				break;

			case PCMD_UNLOCK_STAGES_WITH_DIFFICULTY:
				while(cur < cmdsize) {
					uint16_t stg = SDL_ReadLE16(vfile);
					uint8_t dflags = SDL_ReadU8(vfile);
					StageInfo *info = stage_get(stg);

					for(unsigned int diff = D_Easy; diff <= D_Lunatic; ++diff) {
						if(dflags & (unsigned int)pow(2, diff - D_Easy)) {
							StageProgress *p = stage_get_progress_from_info(info, diff, true);
							if(p) {
								p->unlocked = true;
							}
						}
					}

					cur += 3;
				}
				break;

			case PCMD_HISCORE:
				progress.hiscore = SDL_ReadLE32(vfile);
				break;

			default:
				log_warn("Unknown command %i (%u bytes). Will preserve as-is and not interpret.", cmd, cmdsize);

				UnknownCmd *c = create_element((void**)&progress.unknown, sizeof(UnknownCmd));
				c->cmd = cmd;
				c->size = cmdsize;
				c->data = malloc(sizeof(cmdsize));
				SDL_RWread(vfile, c->data, c->size, 1);

				break;
		}
	}

	free(buf);
	SDL_RWclose(vfile);
}

typedef void (*cmd_preparefunc_t)(size_t*, void**);
typedef void (*cmd_writefunc_t)(SDL_RWops*, void**);

typedef struct cmd_writer_t {
	cmd_preparefunc_t prepare;
	cmd_writefunc_t write;
	void *data;
} cmd_writer_t;

#define CMD_HEADER_SIZE 3

//
//	PCMD_UNLOCK_STAGES
//

static void progress_prepare_cmd_unlock_stages(size_t *bufsize, void **arg) {
	int num_unlocked = 0;

	for(StageInfo *stg = stages; stg->procs; ++stg) {
		StageProgress *p = stage_get_progress_from_info(stg, D_Any, false);
		if(p && p->unlocked) {
			++num_unlocked;
		}
	}

	if(num_unlocked) {
		*bufsize += CMD_HEADER_SIZE;
		*bufsize += num_unlocked * 2;
	}

	*arg = (void*)(intptr_t)num_unlocked;
}

static void progress_write_cmd_unlock_stages(SDL_RWops *vfile, void **arg) {
	int num_unlocked = (intptr_t)*arg;

	if(!num_unlocked) {
		return;
	}

	SDL_WriteU8(vfile, PCMD_UNLOCK_STAGES);
	SDL_WriteLE16(vfile, num_unlocked * 2);

	for(StageInfo *stg = stages; stg->procs; ++stg) {
		StageProgress *p = stage_get_progress_from_info(stg, D_Any, false);
		if(p && p->unlocked) {
			SDL_WriteLE16(vfile, stg->id);
		}
	}
}

//
//	PCMD_UNLOCK_STAGES_WITH_DIFFICULTY
//

static void progress_prepare_cmd_unlock_stages_with_difficulties(size_t *bufsize, void **arg) {
	int num_unlocked = 0;

	for(StageInfo *stg = stages; stg->procs; ++stg) {
		if(stg->difficulty)
			continue;

		bool unlocked = false;

		for(Difficulty diff = D_Easy; diff <= D_Lunatic; diff = static_cast<Difficulty>(static_cast<int>(diff) + 1)) {
			StageProgress *p = stage_get_progress_from_info(stg, diff, false);
			if(p && p->unlocked) {
				unlocked = true;
			}
		}

		if(unlocked) {
			++num_unlocked;
		}
	}

	if(num_unlocked) {
		*bufsize += CMD_HEADER_SIZE;
		*bufsize += num_unlocked * 3;
	}

	*arg = (void*)(intptr_t)num_unlocked;
}

static void progress_write_cmd_unlock_stages_with_difficulties(SDL_RWops *vfile, void **arg) {
	int num_unlocked = (intptr_t)*arg;

	if(!num_unlocked) {
		return;
	}

	SDL_WriteU8(vfile, PCMD_UNLOCK_STAGES_WITH_DIFFICULTY);
	SDL_WriteLE16(vfile, num_unlocked * 3);

	for(StageInfo *stg = stages; stg->procs; ++stg) {
		if(stg->difficulty)
			continue;

		uint8_t dflags = 0;

		for(Difficulty diff = D_Easy; diff <= D_Lunatic; diff = static_cast<Difficulty>(static_cast<int>(diff) + 1)) {
			StageProgress *p = stage_get_progress_from_info(stg, diff, false);
			if(p && p->unlocked) {
				dflags |= (unsigned int)pow(2, diff - D_Easy);
			}
		}

		if(dflags) {
			SDL_WriteLE16(vfile, stg->id);
			SDL_WriteU8(vfile, dflags);
		}
	}
}

//
//	PCMD_HISCORE
//

static void progress_prepare_cmd_hiscore(size_t *bufsize, void **arg) {
	*bufsize += CMD_HEADER_SIZE + sizeof(uint32_t);
}

static void progress_write_cmd_hiscore(SDL_RWops *vfile, void **arg) {
	SDL_WriteU8(vfile, PCMD_HISCORE);
	SDL_WriteLE16(vfile, sizeof(uint32_t));
	SDL_WriteLE32(vfile, progress.hiscore);
}

//
//	Copy unhandled commands from the original file
//

static void progress_prepare_cmd_unknown(size_t *bufsize, void **arg) {
	for(UnknownCmd *c = progress.unknown; c; c = c->next) {
		*bufsize += CMD_HEADER_SIZE + c->size;
	}
}

static void progress_write_cmd_unknown(SDL_RWops *vfile, void **arg) {
	for(UnknownCmd *c = progress.unknown; c; c = c->next) {
		SDL_WriteU8(vfile, c->cmd);
		SDL_WriteLE16(vfile, c->size);
		SDL_RWwrite(vfile, c->data, c->size, 1);
	}
}

//
//	Test
//

__attribute__((unused)) static void progress_prepare_cmd_test(size_t *bufsize, void **arg) {
	*bufsize += CMD_HEADER_SIZE + sizeof(uint32_t);
}

__attribute__((unused)) static void progress_write_cmd_test(SDL_RWops *vfile, void **arg) {
	SDL_WriteU8(vfile, 0x7f);
	SDL_WriteLE16(vfile, sizeof(uint32_t));
	SDL_WriteLE32(vfile, 0xdeadbeef);
}

static void progress_write(SDL_RWops *file) {
	size_t bufsize = 0;
	SDL_RWwrite(file, progress_magic_bytes, 1, sizeof(progress_magic_bytes));

	cmd_writer_t cmdtable[] = {
		{progress_prepare_cmd_unlock_stages, progress_write_cmd_unlock_stages, NULL},
		{progress_prepare_cmd_unlock_stages_with_difficulties, progress_write_cmd_unlock_stages_with_difficulties, NULL},
		{progress_prepare_cmd_hiscore, progress_write_cmd_hiscore, NULL},
		// {progress_prepare_cmd_test, progress_write_cmd_test, NULL},
		{progress_prepare_cmd_unknown, progress_write_cmd_unknown, NULL},
		{NULL}
	};

	for(cmd_writer_t *w = cmdtable; w->prepare; ++w) {
		w->prepare(&bufsize, &w->data);
	}

	if(!bufsize) {
		return;
	}

	uint8_t *buf = malloc(bufsize);
	memset(buf, 0x7f, bufsize);
	SDL_RWops *vfile = SDL_RWFromMem(buf, bufsize);

	for(cmd_writer_t *w = cmdtable; w->prepare; ++w) {
		w->write(vfile, &w->data);
	}

	if(SDL_RWtell(vfile) != bufsize) {
		free(buf);
		log_fatal("Buffer is inconsistent");
		return;
	}

	uint32_t cs = progress_checksum(buf, bufsize);
	// no byteswapping here
	SDL_RWwrite(file, &cs, 4, 1);

	if(!SDL_RWwrite(file, buf, bufsize, 1)) {
		log_fatal("SDL_RWwrite() failed: %s", SDL_GetError());
		free(buf);
		return;
	}

	free(buf);
	SDL_RWclose(vfile);
}

#ifdef PROGRESS_UNLOCK_ALL
static void progress_unlock_all(void) {
	StageInfo *stg;

	for(stg = stages; stg->procs; ++stg) {
		for(Difficulty diff = D_Any; diff <= D_Lunatic; ++diff) {
			StageProgress *p = stage_get_progress_from_info(stg, diff, true);
			if(p) {
				p->unlocked = true;
			}
		}
	}
}
#endif

void progress_load(void) {
	memset(&progress, 0, sizeof(GlobalProgress));

#ifdef PROGRESS_UNLOCK_ALL
	progress_unlock_all();
	progress_save();
#endif

	char *p = progress_getpath();
	SDL_RWops *file = SDL_RWFromFile(p, "rb");

	if(!file) {
		log_warn("Couldn't open the progress file: %s", SDL_GetError());
		free(p);
		return;
	}

	free(p);
	progress_read(file);
	SDL_RWclose(file);
}

void progress_save(void) {
	char *p = progress_getpath();
	SDL_RWops *file = SDL_RWFromFile(p, "wb");

	if(!file) {
		log_warn("Couldn't open the progress file: %s", SDL_GetError());
		free(p);
		return;
	}

	free(p);
	progress_write(file);
	SDL_RWclose(file);
}

static void delete_unknown_cmd(void **dest, void *elem) {
	UnknownCmd *cmd = elem;
	free(cmd->data);
	delete_element(dest, cmd);
}

void progress_unload(void) {
	delete_all_elements((void**)&progress.unknown, delete_unknown_cmd);
}
