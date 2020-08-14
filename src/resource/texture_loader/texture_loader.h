/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#ifndef IGUARD_resource_texture_loader_texture_loader_h
#define IGUARD_resource_texture_loader_texture_loader_h

#include "taisei.h"

#include "renderer/api.h"
#include "resource/resource.h"
#include "resource/texture.h"

typedef struct TextureLoadData {
	Pixmap pixmap;
	Pixmap pixmap_alphamap;

	uint num_mipmaps;
	Pixmap *mipmaps;

	TextureParams params;

	struct {
		// NOTE: not bitfields because we take pointers to these
		bool linearize;
		bool multiply_alpha;
		bool apply_alphamap;
	} preprocess;

	// NOTE: Despite being a PixmapFormat, this is also used to pick a proper TextureType later. Irrelevant for compressed textures, unless decompression fallback is used.
	PixmapFormat preferred_format;

	const char *tex_src_path;
	const char *alphamap_src_path;
	char *tex_src_path_allocated;
	char *alphamap_src_path_allocated;

	ResourceLoadState *st;
} TextureLoadData;

char *texture_loader_source_path(const char *basename);
char *texture_loader_path(const char *basename);
bool texture_loader_check_path(const char *path);

void texture_loader_stage1(ResourceLoadState *st);
void texture_loader_cleanup_stage1(TextureLoadData *ld);
void texture_loader_cleanup_stage2(TextureLoadData *ld);
void texture_loader_cleanup(TextureLoadData *ld);
void texture_loader_failed(TextureLoadData *ld);
void texture_loader_continue(TextureLoadData *ld);
void texture_loader_unload(void *vtexture);

bool texture_loader_try_set_texture_type(
	TextureLoadData *ld,
	TextureType tex_type,
	PixmapFormat px_fmt,
	PixmapOrigin px_org,
	bool srgb_fallback,
	TextureTypeQueryResult *out_qr
) attr_nodiscard;

bool texture_loader_set_texture_type_uncompressed(
	TextureLoadData *ld,
	TextureType tex_type,
	PixmapFormat px_fmt,
	PixmapOrigin px_org,
	TextureTypeQueryResult *out_qr
) attr_nodiscard;

bool texture_loader_prepare_pixmaps(
	TextureLoadData *ld,
	Pixmap *pm_main,
	Pixmap *pm_alphamap,
	TextureType tex_type,
	TextureFlags tex_flags
) attr_nodiscard;

#endif // IGUARD_resource_texture_loader_texture_loader_h
