/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "native.h"
#include "global.h"
#include <stdlib.h>
#include <string.h>

#define CFG_DIR "/.taisei"
#define SCR_DIR "/screenshots"

char *conf_path = NULL;
char *scr_path = NULL;

const char *get_prefix() {
	return FILE_PREFIX;
}

const char *get_config_path() {
	if(conf_path == NULL) {
		conf_path = malloc(strlen(CFG_DIR) + strlen(getenv("HOME")) + 1);
		strcpy(conf_path, getenv("HOME"));
		strcat(conf_path, CFG_DIR);
	}
	
	return conf_path;
}

const char *get_screenshots_path() {
	if(scr_path == NULL) {
		scr_path = malloc(strlen(SCR_DIR) + strlen(get_config_path()) + 1);
		strcpy(scr_path, get_config_path());
		strcat(scr_path, SCR_DIR);
	}
	
	return scr_path;
}

