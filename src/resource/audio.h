/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef AUDIO_H
#define AUDIO_H

#include <SDL_mixer.h>

#define SNDCHAN_COUNT 100

typedef enum {
	ST_SOUND,
	ST_MUSIC
} sound_type_t;

struct Sound;

typedef struct Sound {
	struct Sound *next;
	struct Sound *prev;

	int lastplayframe;
	sound_type_t type;
	union {
		Mix_Chunk *sound;
		Mix_Music *music;
	};
	char *name;
} Sound;

Sound *load_sound(char *filename);
Sound *load_sound_or_bgm(char *filename, Sound **dest, sound_type_t type);

#define play_sound(name) play_sound_p(name, 0);
#define play_ui_sound(name) play_sound_p(name, 1);

void play_sound_p(char *name, int unconditional);

Sound *get_snd(Sound *source, char *name);

void delete_sound(void **snds, void *snd);
void delete_sounds(void);

void set_sfx_volume(float gain);

int init_sfx(void);
void shutdown_sfx(void);

int init_mixer_if_needed(void);
void unload_mixer_if_needed(void);

#endif
