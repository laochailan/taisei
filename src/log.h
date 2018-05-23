/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include <SDL.h>

typedef enum LogLevel {
	LOG_NONE = 0,
	LOG_DEBUG = 1,
	LOG_INFO = 2,
	LOG_WARN = 4,
	LOG_FATAL = 8,

	LOG_SPAM = LOG_DEBUG | LOG_INFO,
	LOG_ALERT = LOG_WARN | LOG_FATAL,

	LOG_ALL = LOG_SPAM | LOG_ALERT,
} LogLevel;

#ifndef LOG_DEFAULT_LEVELS
	#define LOG_DEFAULT_LEVELS LOG_ALL
#endif

#ifndef LOG_DEFAULT_LEVELS_FILE
	#define LOG_DEFAULT_LEVELS_FILE LOG_ALL
#endif

#ifndef LOG_DEFAULT_LEVELS_CONSOLE
	#ifdef DEBUG
		#define LOG_DEFAULT_LEVELS_CONSOLE LOG_ALL
	#else
		#define LOG_DEFAULT_LEVELS_CONSOLE LOG_ALERT
	#endif
#endif

#ifndef LOG_DEFAULT_LEVELS_STDOUT
	#define LOG_DEFAULT_LEVELS_STDOUT LOG_SPAM
#endif

#ifndef LOG_DEFAULT_LEVELS_STDERR
	#define LOG_DEFAULT_LEVELS_STDERR LOG_ALERT
#endif

#ifndef LOG_DEFAULT_LEVELS_BACKTRACE
	#ifdef LOG_ENABLE_BACKTRACE
		#define LOG_DEFAULT_LEVELS_BACKTRACE LOG_FATAL
	#else
		#define LOG_DEFAULT_LEVELS_BACKTRACE LOG_NONE
	#endif
#endif

#ifndef LOG_BACKTRACE_SIZE
	#define LOG_BACKTRACE_SIZE 32
#endif

void log_init(LogLevel lvls, LogLevel backtrace_lvls);
void log_shutdown(void);
void log_add_output(LogLevel levels, SDL_RWops *output);
void log_backtrace(LogLevel lvl);
LogLevel log_parse_levels(LogLevel lvls, const char *lvlmod);
bool log_initialized(void);

#ifdef DEBUG
	#define log_debug(...) _taisei_log(LOG_DEBUG, false, __func__, __VA_ARGS__)
#else
	#define log_debug(...)
#endif

#define log_info(...) _taisei_log(LOG_INFO, false, __func__, __VA_ARGS__)
#define log_warn(...) _taisei_log(LOG_WARN, false, __func__, __VA_ARGS__)
#define log_fatal(...) _taisei_log_fatal(LOG_FATAL, __func__, __VA_ARGS__)
#define log_custom(lvl, ...) _taisei_log(lvl, false, __func__, __VA_ARGS__)

#define log_sdl_error(funcname) log_warn("%s() failed: %s", funcname, SDL_GetError())

//
// don't call these directly, use the macros
//

void _taisei_log(LogLevel lvl, bool is_backtrace, const char *funcname, const char *fmt, ...)
	attr_printf(4, 5);

noreturn void _taisei_log_fatal(LogLevel lvl, const char *funcname, const char *fmt, ...)
	attr_printf(3, 4);

#ifdef DEBUG
	#undef UNREACHABLE
	#define UNREACHABLE log_fatal("This code should never be reached")
#endif
