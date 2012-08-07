/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2011, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */
 
#include "global.h"
#include "menu.h"
#include "options.h"
#include "stage.h"
#include "stageselect.h"
#include "mainmenu.h"

void backtomain(void*);

void create_stage_menu(MenuData *m) {
	char title[STGMENU_MAX_TITLE_LENGTH];
	int i;
	
	create_menu(m);
	m->type = MT_Persistent;
	m->abortable = True;
	m->abortaction = backtomain;
	m->abortarg = m;
	// TODO: I think ALL menus should use the title field, but I don't want to screw with it right now.
	m->title = "Stage Select";
	
	for(i = 0; stages[i].loop; ++i) if(!stages[i].hidden) {
		snprintf(title, STGMENU_MAX_TITLE_LENGTH, "%s", stages[i].title);
		add_menu_entry(m, title, start_story, &(stages[i]));
	}
	
	add_menu_separator(m);
	add_menu_entry(m, "Back", backtomain, m);
}

void draw_stage_menu(MenuData *m) {
	draw_options_menu_bg(m);
	draw_text(AL_Right, (stringwidth(m->title, _fonts.mainmenu) + 10) * (1-m->fade), 30, m->title, _fonts.mainmenu);
	
	glPushMatrix();
	glTranslatef(100, 100 + (((m->ecount * 20 + 290) > SCREEN_W)? min(0, SCREEN_H * 0.7 - 100 - m->drawdata[2]) : 0), 0);
	
	/*
	glPushMatrix();
	glTranslatef(SCREEN_W/2 - 100, m->drawdata[2], 0);
	glScalef(SCREEN_W - 200, 20, 1);
	glColor4f(0,0,0,0.5);
	draw_quad();
	glPopMatrix();
	*/
	
	Texture *bg = get_tex("part/smoke");
	glPushMatrix();
	glTranslatef(m->drawdata[0], m->drawdata[2], 0);
	glScalef(m->drawdata[1]/100.0, 0.2, 1);
	glRotatef(m->frames*2,0,0,1);
	glColor4f(0,0,0,0.5);
	draw_texture_p(0,0,bg);
	glPopMatrix();
	
	MenuEntry *s = &(m->entries[m->cursor]);
	m->drawdata[0] += ((s->draw? SCREEN_W/2 - 100 : (stringwidth(s->name, _fonts.mainmenu)/2 - s->drawdata*1.5)) - m->drawdata[0])/10.0;
	m->drawdata[1] += ((s->draw? (SCREEN_W - 200)*0.85 : stringwidth(s->name, _fonts.mainmenu)) - m->drawdata[1])/10.0;
	m->drawdata[2] += (20*m->cursor - m->drawdata[2])/10.0;
	
	int i;
	for(i = 0; i < m->ecount; i++) {
		MenuEntry *e = &(m->entries[i]);
		
		e->drawdata += 0.2 * (10*(i == m->cursor) - e->drawdata);
		float a = e->drawdata * 0.1;
		
		if(e->action == NULL)
			glColor4f(0.5, 0.5, 0.5, 0.5);
		else {
			//glColor4f(0.7 + 0.3 * (1-a), 1, 1, 0.7 + 0.3 * a);
			float ia = 1-a;
			glColor4f(0.9 + ia * 0.1, 0.6 + ia * 0.4, 0.2 + ia * 0.8, 0.7 + 0.3 * a);
		}
		
		if(e->draw)
			e->draw(e, i, m->ecount);
		else if(e->name)
			draw_text(AL_Left, 20 - e->drawdata, 20*i, e->name, _fonts.standard);
	}
	
	glPopMatrix();
	
	fade_out(m->fade);
}

int stage_menu_loop(MenuData *m) {
	return menu_loop(m, NULL, draw_stage_menu);
}
