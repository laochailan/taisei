/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "stage.h"

#include <SDL/SDL.h>
#include <time.h>
#include "global.h"
#include "video.h"
#include "replay.h"
#include "config.h"
#include "player.h"
#include "menu/ingamemenu.h"
#include "menu/savereplay.h"

StageInfo stages[] = {	
	{1, stage0_loop, False, "Stage 1", "Misty Lake", {1, 1, 1}},
	{2, stage1_loop, False, "Stage 2", "Walk Along the Border", {1, 1, 1}},
	{3, stage2_loop, False, "Stage 3", "Through the Tunnel of Light", {0, 0, 0}},
	{4, stage3_loop, False, "Stage 4", "Forgotten Mansion", {0, 0, 0}},
	{5, stage4_loop, False, "Stage 5", "Climbing the Tower of Babel", {1, 1, 1}},
	
	{0, NULL, False, NULL, NULL}
};

// NOTE: This returns the stage BY ID, not by the array index!
StageInfo* stage_get(int n) {
	int i;
	for(i = 0; stages[i].loop; ++i)
		if(stages[i].id == n)
			return &(stages[i]);
	return NULL;
}

void stage_start() {
	global.timer = 0;
	global.frames = 0;
	global.game_over = 0;
	global.points = 0;
	global.nostagebg = False;
	
	global.fps.show_fps = 60;
	global.fps.fpstime = SDL_GetTicks();
	
	global.plr.recovery = 0;
}

void stage_ingamemenu() {
	if(!global.menu)
		global.menu = create_ingame_menu();
	else
		global.menu->quit = 1;
}

void replay_input() {
	if(global.menu) {
		menu_input(global.menu);
		return;
	}
	
	SDL_Event event;
	while(SDL_PollEvent(&event)) {
		int sym = event.key.keysym.sym;
		
		switch(event.type) {
			case SDL_KEYDOWN:
				if(sym == SDLK_ESCAPE)
					stage_ingamemenu();
				break;
				
			case SDL_QUIT:
				exit(1);
				break;
		}
	}
	
// 	I know this loop is not (yet) optimal - consider it a sketch
	int i;
	for(i = 0; i < global.replay.ecount; ++i) {
		ReplayEvent *e = &(global.replay.events[i]);
		
		if(e->frame == global.frames) switch(e->type) {
			case EV_OVER:
				global.game_over = GAMEOVER_ABORT;
				break;
			
			default:
				if(global.dialog && e->type == EV_PRESS && (e->key == KEY_SHOT || e->key == KEY_BOMB))
					page_dialog(&global.dialog);
				else
					player_event(&global.plr, e->type, e->key);
				break;
		}
	}
	
	player_applymovement(&global.plr);
}

void stage_input() {
	if(global.menu) {
		menu_input(global.menu);
		return;
	}
	
	SDL_Event event;
	while(SDL_PollEvent(&event)) {
		int sym = event.key.keysym.sym;
		int key = config_sym2key(sym);
		global_processevent(&event);
		
		switch(event.type) {
			case SDL_KEYDOWN:
				if(global.dialog && (key == KEY_SHOT || key == KEY_BOMB)) {
					page_dialog(&global.dialog);
					replay_event(&global.replay, EV_PRESS, key);
				} else if(sym == SDLK_ESCAPE) {
					stage_ingamemenu();
				} else {
					player_event(&global.plr, EV_PRESS, key);
					replay_event(&global.replay, EV_PRESS, key);
				}
				
				break;
				
			case SDL_KEYUP:
				player_event(&global.plr,EV_RELEASE, key);
				replay_event(&global.replay, EV_RELEASE, key);
				break;
			
			case SDL_QUIT:
				exit(1);
				break;
		}
	}
	
	player_applymovement(&global.plr);
}

void draw_hud() {
	draw_texture(SCREEN_W/2.0, SCREEN_H/2.0, "hud");	
	
	char buf[16];
	int i;
	
	glPushMatrix();
	glTranslatef(615,0,0);
	
	for(i = 0; i < global.plr.lifes; i++)
	  draw_texture(16*i,167, "star");

	for(i = 0; i < global.plr.bombs; i++)
	  draw_texture(16*i,200, "star");
	
	sprintf(buf, "%.2f", global.plr.power);
	draw_text(AL_Center, 10, 236, buf, _fonts.standard);
	
	sprintf(buf, "%i", global.points);
	draw_text(AL_Center, 13, 49, buf, _fonts.standard);
	
	glPopMatrix();
	
	sprintf(buf, "%i fps", global.fps.show_fps);
	draw_text(AL_Right, SCREEN_W, SCREEN_H-20, buf, _fonts.standard);	
	
	if(global.boss)
		draw_texture(VIEWPORT_X+creal(global.boss->pos), 590, "boss_indicator");
}

void stage_draw(StageInfo *info, StageRule bgdraw, ShaderRule *shaderrules, int time) {
	if(!tconfig.intval[NO_SHADER]) {
		glBindFramebuffer(GL_FRAMEBUFFER, resources.fbg[0].fbo);
		if(!global.menu)
			glViewport(0,0,SCREEN_W,SCREEN_H);
	}
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glPushMatrix();	
	glTranslatef(-(VIEWPORT_X+VIEWPORT_W/2.0), -(VIEWPORT_Y+VIEWPORT_H/2.0),0);
	glEnable(GL_DEPTH_TEST);
		
	if(tconfig.intval[NO_STAGEBG] == 2 && global.fps.show_fps < tconfig.intval[NO_STAGEBG_FPSLIMIT]
		&& !global.nostagebg) {
		
		printf("stage_draw(): !- Stage background has been switched off due to low frame rate. You can change that in the options.\n");
		global.nostagebg = True;
	}
	
	if(tconfig.intval[NO_STAGEBG] == 1)
		global.nostagebg = True;
	
	if(!global.nostagebg && !global.menu)
		bgdraw();
		
	glPopMatrix();	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	set_ortho();

	if(!global.menu) {
		glPushMatrix();
		glTranslatef(VIEWPORT_X,VIEWPORT_Y,0);
		
		if(!tconfig.intval[NO_SHADER])
			apply_bg_shaders(shaderrules);

		if(global.boss) {
			glPushMatrix();
			glTranslatef(creal(global.boss->pos), cimag(global.boss->pos), 0);
			
			if(!(global.frames % 5)) {
				complex offset = (frand()-0.5)*50 + (frand()-0.5)*20I;
				create_particle3c("boss_shadow", -20I, rgba(0.2,0.35,0.5,0.5), EnemyFlareShrink, enemy_flare, 50, (-100I-offset)/(50.0+frand()*10), add_ref(global.boss));
			}
			
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
						
			glRotatef(global.frames*4.0, 0, 0, -1);
			float f = 0.8+0.1*sin(global.frames/8.0);
			glScalef(f,f,f);
			draw_texture(0,0,"boss_circle");
			
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			
			glPopMatrix();
		}
		
		player_draw(&global.plr);

		draw_items();		
		draw_projectiles(global.projs);
		
		
		draw_projectiles(global.particles);
		draw_enemies(global.enemies);
		draw_lasers();
				
		if(global.boss)
			draw_boss(global.boss);

		if(global.dialog)
			draw_dialog(global.dialog);
		
		draw_stage_title(info);
		
		if(!tconfig.intval[NO_SHADER]) {
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, video.current.width, video.current.height);
			glPushMatrix();
			if(global.plr.cha == Marisa && global.plr.shot == MarisaLaser && global.frames - global.plr.recovery < 0)
				glTranslatef(8*sin(global.frames),8*sin(global.frames+3),0);

			draw_fbo_viewport(&resources.fsec);

			glPopMatrix();
		}
		
		glPopMatrix();
	}
	
	if(global.frames < 4*FADE_TIME)
		fade_out(1.0 - global.frames/(float)(4*FADE_TIME));
	if(global.timer > time - 4*FADE_TIME) {
		fade_out((global.timer - time + 4*FADE_TIME)/(float)(4*FADE_TIME));
	}
	
	if(global.menu) {
		glPushMatrix();
		glTranslatef(VIEWPORT_X,VIEWPORT_Y,0);
		draw_ingame_menu(global.menu);
		glPopMatrix();
	}
	
	draw_hud();
	
	if(global.menu) {
		// horrible hacks because we have no sane transitions between ingame menus
		
		if(REPLAY_ASKSAVE) {
			if(global.menu->context && global.menu->quit == 1) {
				fade_out(global.menu->fade);
			}
		} else {
			fade_out(global.menu->fade);
		}
	}
}

void apply_bg_shaders(ShaderRule *shaderrules) {
	int fbonum = 0;
	int i;
	
	if(global.boss && global.boss->current && global.boss->current->draw_rule) {
		glBindFramebuffer(GL_FRAMEBUFFER, resources.fbg[0].fbo);
		global.boss->current->draw_rule(global.boss, global.frames - global.boss->current->starttime);
		
		glPushMatrix();
			glTranslatef(creal(global.boss->pos), cimag(global.boss->pos), 0);
			glRotatef(global.frames*7.0, 0, 0, -1);
			
			int t;
			if((t = global.frames - global.boss->current->starttime) < 0) {
				float f = 1.0 - t/(float)ATTACK_START_DELAY;
				glScalef(f,f,f);
			}
			
			draw_texture(0,0,"boss_spellcircle0");
		glPopMatrix();
		
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	} else if(!global.nostagebg) {		
		for(i = 0; shaderrules != NULL && shaderrules[i] != NULL; i++) {
			glBindFramebuffer(GL_FRAMEBUFFER, resources.fbg[!fbonum].fbo);
			shaderrules[i](fbonum);

			fbonum = !fbonum;
		}
	}
	
	glBindFramebuffer(GL_FRAMEBUFFER, resources.fsec.fbo);
	
	if(global.boss) { // Boss background shader
		Shader *shader = get_shader("boss_zoom");
		glUseProgram(shader->prog);
		
		complex fpos = VIEWPORT_H*I + conj(global.boss->pos) + (VIEWPORT_X + VIEWPORT_Y*I);
		complex pos = fpos + 15*cexp(I*global.frames/4.5);
		
		glUniform2f(uniloc(shader, "blur_orig"),
					creal(pos)/resources.fbg[fbonum].nw, cimag(pos)/resources.fbg[fbonum].nh);
		glUniform2f(uniloc(shader, "fix_orig"),
					creal(fpos)/resources.fbg[fbonum].nw, cimag(fpos)/resources.fbg[fbonum].nh);
		glUniform1f(uniloc(shader, "blur_rad"), 0.2+0.025*sin(global.frames/15.0));
		glUniform1f(uniloc(shader, "rad"), 0.24);
		glUniform1f(uniloc(shader, "ratio"), (float)resources.fbg[fbonum].nh/resources.fbg[fbonum].nw);
	}
	
	draw_fbo_viewport(&resources.fbg[fbonum]);
	
	glUseProgram(0);
	
	if(global.frames - global.plr.recovery < 0) {
		float t = BOMB_RECOVERY - global.plr.recovery + global.frames;
		float fade = 1;
	
		if(t < BOMB_RECOVERY/6)
			fade = t/BOMB_RECOVERY*6;
		
		if(t > BOMB_RECOVERY/4*3)
			fade = 1-t/BOMB_RECOVERY*4 + 3;
	
		glPushMatrix();
		glTranslatef(-30,-30,0);
		fade_out(fade*0.6);
		glPopMatrix();
	}
}

void stage_logic(int time) {
	if(global.menu) {
		ingame_menu_logic(&global.menu);
		return;
	}
	
	player_logic(&global.plr);
	
	process_enemies(&global.enemies);
	process_projectiles(&global.projs, True);
	process_items();
	process_lasers();
	process_projectiles(&global.particles, False);
	
	if(global.boss && !global.dialog) {
		process_boss(global.boss);
		if(global.boss->dmg > global.boss->attacks[global.boss->acount-1].dmglimit)
			boss_death(&global.boss);
	}
	
	global.frames++;
	
	if(!global.dialog && !global.boss)
		global.timer++;
	
	if(global.timer >= time)
		global.game_over = GAMEOVER_WIN;
	
}

void stage_end() {
	delete_projectiles(&global.projs);
	delete_projectiles(&global.particles);
	delete_enemies(&global.enemies);
	delete_items();
	delete_lasers();
	
	if(global.menu) {
		destroy_menu(global.menu);
		global.menu = NULL;
	}
	
	if(global.dialog) {
		delete_dialog(global.dialog);
		global.dialog = NULL;
	}
	
	if(global.boss) {
		free_boss(global.boss);
		global.boss = NULL;
	}
}

void stage_loop(StageInfo* info, StageRule start, StageRule end, StageRule draw, StageRule event, ShaderRule *shaderrules, int endtime) {
	if(global.game_over == GAMEOVER_WIN) {
		global.game_over = 0;
	} else if(global.game_over) {
		return;
	}
	
	int seed = time(0);
	tsrand_seed_p(&global.rand_game, seed);
	
	if(global.replaymode == REPLAY_RECORD) {
		replay_destroy(&global.replay);
		if(!global.plr.continues)
			replay_init(&global.replay, info, seed, &global.plr);
		printf("Random seed: %d\n", seed);
	} else {
		printf("REPLAY_PLAY mode: %d events\n", global.replay.ecount);
		
		tsrand_seed_p(&global.rand_game, global.replay.seed);
		printf("Random seed: %d\n", global.replay.seed);
		
		global.diff			= global.replay.diff;
		global.points		= global.replay.points;
		
		global.plr.shot		= global.replay.plr_shot;
		global.plr.cha		= global.replay.plr_char;
		global.plr.pos		= global.replay.plr_pos;
		global.plr.lifes	= global.replay.plr_lifes;
		global.plr.bombs	= global.replay.plr_bombs;
		global.plr.power	= global.replay.plr_power;
	}
	
	tsrand_switch(&global.rand_game);
	stage_start();
	start();
	
	while(global.game_over <= 0) {
		if(!global.boss && !global.dialog && !global.menu)
			event();
		((global.replaymode == REPLAY_PLAY)? replay_input : stage_input)();
		stage_logic(endtime);
		
		calc_fps(&global.fps);
		
		tsrand_switch(&global.rand_visual);
		stage_draw(info, draw, shaderrules, endtime);
		tsrand_switch(&global.rand_game);
		
		SDL_GL_SwapBuffers();
		frame_rate(&global.lasttime);
	}
	
	if(global.replaymode == REPLAY_RECORD) {
		replay_event(&global.replay, EV_OVER, 0);
		
		if(REPLAY_ASKSAVE) {
			global.menu = create_saverpy_menu();
			while(global.menu) {
				stage_draw(info, draw, shaderrules, endtime);
				ingame_menu_logic(&global.menu);
				
				if(!global.menu)
					break;
								
				menu_input(global.menu);
				SDL_GL_SwapBuffers();
				frame_rate(&global.lasttime);
			}
		}
		
		if(global.replay.active && tconfig.intval[SAVE_RPY] == 1)
			save_rpy(NULL);
	}
	
	end();
	stage_end();
	tsrand_switch(&global.rand_visual);
}

void draw_stage_title(StageInfo *info) {
	int t = global.timer;
	int i;
	float f = 0;
	if(t < 30 || t > 220)
		return;
	
	if((i = abs(t-135)) >= 50) {
		i -= 50;
		f = 1/35.0*i;		
	}
	
	glColor4f(info->titleclr.r,info->titleclr.g,info->titleclr.b,1.0-f);
	
	draw_text(AL_Center, VIEWPORT_W/2, VIEWPORT_H/2-40, info->title, _fonts.mainmenu);
	draw_text(AL_Center, VIEWPORT_W/2, VIEWPORT_H/2, info->subtitle, _fonts.standard);
	
	glColor4f(1,1,1,1);
}
