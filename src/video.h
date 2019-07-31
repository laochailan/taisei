/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_video_h
#define IGUARD_video_h

#include "taisei.h"

// FIXME: This is just for IntRect, which probably should be placed elsewhere.
#include "util/geometry.h"

#include <SDL.h>

#define WINFLAGS_IS_FULLSCREEN(f)       ((f) & SDL_WINDOW_FULLSCREEN_DESKTOP)
#define WINFLAGS_IS_FAKE_FULLSCREEN(f)  (WINFLAGS_IS_FULLSCREEN(f) == SDL_WINDOW_FULLSCREEN_DESKTOP)
#define WINFLAGS_IS_REAL_FULLSCREEN(f)  (WINFLAGS_IS_FULLSCREEN(f) == SDL_WINDOW_FULLSCREEN)

#define WINDOW_TITLE "Taisei Project"
#define VIDEO_ASPECT_RATIO ((double)SCREEN_W/SCREEN_H)

enum {
	SCREEN_W = 800,
	SCREEN_H = 600,
};

#define SCREEN_SIZE { SCREEN_W, SCREEN_H }

typedef struct VideoMode {
	int width;
	int height;
} VideoMode;

typedef enum VideoBackend {
	VIDEO_BACKEND_OTHER,
	VIDEO_BACKEND_X11,
	VIDEO_BACKEND_EMSCRIPTEN,
	VIDEO_BACKEND_KMSDRM,
	VIDEO_BACKEND_RPI,
	VIDEO_BACKEND_SWITCH,
} VideoBackend;

typedef struct {
	VideoMode *modes;
	SDL_Window *window;
	uint mcount;
	VideoMode intended;
	VideoMode current;
	VideoBackend backend;
} Video;

typedef enum VideoCapability {
	VIDEO_CAP_FULLSCREEN,
	VIDEO_CAP_EXTERNAL_RESIZE,
	VIDEO_CAP_CHANGE_RESOLUTION,
	VIDEO_CAP_VSYNC_ADAPTIVE,
} VideoCapability;

typedef enum VideoCapabilityState {
	VIDEO_NEVER_AVAILABLE,
	VIDEO_AVAILABLE,
	VIDEO_ALWAYS_ENABLED,
	VIDEO_CURRENTLY_UNAVAILABLE,
} VideoCapabilityState;

extern Video video;

void video_init(void);
void video_shutdown(void);
void video_set_mode(uint display, uint w, uint h, bool fs, bool resizable);
void video_set_fullscreen(bool fullscreen);
void video_get_viewport(FloatRect *vp);
void video_get_viewport_size(float *width, float *height);
bool video_is_fullscreen(void);
bool video_is_resizable(void);
extern VideoCapabilityState (*video_query_capability)(VideoCapability cap);
void video_take_screenshot(void);
void video_swap_buffers(void);
uint video_num_displays(void);
uint video_current_display(void);
void video_set_display(uint idx);
const char *video_display_name(uint id) attr_returns_nonnull;

#endif // IGUARD_video_h
