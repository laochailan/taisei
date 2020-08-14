/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_pixmap_loaders_loaders_h
#define IGUARD_pixmap_loaders_loaders_h

#include "taisei.h"

#include <SDL.h>

#include "../pixmap.h"

typedef struct PixmapLoader {
	bool (*probe)(SDL_RWops *stream);
	bool (*load)(SDL_RWops *stream, Pixmap *pixmap, PixmapFormat preferred_format);
	const char **filename_exts;
} PixmapLoader;

extern PixmapLoader pixmap_loader_png;
extern PixmapLoader pixmap_loader_webp;

#endif // IGUARD_pixmap_loaders_loaders_h
