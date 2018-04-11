/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "build_config.h"
#include "compat.h"

#ifdef TAISEI_BUILDCONF_DEBUG
	#define DEBUG 1
#endif

#ifdef TAISEI_BUILDCONF_LOG_ENABLE_BACKTRACE
	#define LOG_ENABLE_BACKTRACE
#endif

#ifdef TAISEI_BUILDCONF_LOG_FATAL_MSGBOX
	#define LOG_FATAL_MSGBOX
#endif
