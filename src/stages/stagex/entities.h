/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#ifndef IGUARD_stages_stagex_entities_h
#define IGUARD_stages_stagex_entities_h

#include "taisei.h"

#define ENTITIES_STAGEX(X, ...) \
	X(YumemiSlave, __VA_ARGS__) \
	X(BossShield, __VA_ARGS__) \

#endif // IGUARD_stages_stagex_entities_h
