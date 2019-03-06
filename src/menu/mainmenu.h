/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef IGUARD_menu_mainmenu_h
#define IGUARD_menu_mainmenu_h

#include "taisei.h"

#include "menu.h"

MenuData* create_main_menu(void);
void draw_main_menu_bg(MenuData *m);
void draw_main_menu(MenuData *m);
void main_menu_update_practice_menus(void);
void draw_loading_screen(void);
void menu_preload(void);

#endif // IGUARD_menu_mainmenu_h
