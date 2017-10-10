/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "player.h"

#include "projectile.h"
#include "global.h"
#include "plrmodes.h"
#include "stage.h"
#include "stagetext.h"

void player_init(Player *plr) {
	memset(plr, 0, sizeof(Player));
	plr->pos = VIEWPORT_W/2 + I*(VIEWPORT_H-64);
	plr->lives = PLR_START_LIVES;
	plr->bombs = PLR_START_BOMBS;
	plr->deathtime = -1;
	plr->mode = plrmode_find(0, 0);
}

void player_stage_pre_init(Player *plr) {
	plr->recovery = 0;
	plr->respawntime = 0;
	plr->deathtime = -1;
	plr->graze = 0;
	plr->axis_lr = 0;
	plr->axis_ud = 0;
	plrmode_preload(plr->mode);
}

void player_stage_post_init(Player *plr) {
	// TODO: remove handle_fullpower from player_set_power and get rid of this hack
	short power = global.plr.power;
	global.plr.power = -1;
	delete_enemies(&global.plr.slaves);
	player_set_power(&global.plr, power, false);

	assert(plr->mode != NULL);
	aniplayer_create(&plr->ani, get_ani(plr->mode->character->player_sprite_name));

	// ensure the essential callbacks are there. other code tests only for the optional ones
	assert(plr->mode->procs.shot != NULL);
	assert(plr->mode->procs.bomb != NULL);
}

static void player_full_power(Player *plr) {
	play_sound("full_power");
	stage_clear_hazards(false);
	stagetext_add("Full Power!", VIEWPORT_W * 0.5 + VIEWPORT_H * 0.33 * I, AL_Center, _fonts.mainmenu, rgb(1, 1, 1), 0, 60, 20, 20);
}

bool player_set_power(Player *plr, short npow, bool handle_fullpower) {
	npow = clamp(npow, 0, PLR_MAX_POWER);

	if(plr->mode->procs.power) {
		plr->mode->procs.power(plr, npow);
	}

	int oldpow = plr->power;
	plr->power = npow;

	if(plr->power == PLR_MAX_POWER && oldpow < PLR_MAX_POWER && handle_fullpower) {
		player_full_power(plr);
	}

	return oldpow != plr->power;
}

void player_move(Player *plr, complex delta) {
	float speed = 0.01*VIEWPORT_W;

	if(plr->inputflags & INFLAG_FOCUS) {
		speed /= 2.0;
	}

	if(plr->mode->procs.speed_mod) {
		speed = plr->mode->procs.speed_mod(plr, speed);
	}

	complex opos = plr->pos - VIEWPORT_W/2.0 - VIEWPORT_H/2.0*I;
	complex npos = opos + delta*speed;

	Animation *ani = plr->ani.ani;

	bool xfac = fabs(creal(npos)) < fabs(creal(opos)) || fabs(creal(npos)) < VIEWPORT_W/2.0 - ani->w/2;
	bool yfac = fabs(cimag(npos)) < fabs(cimag(opos)) || fabs(cimag(npos)) < VIEWPORT_H/2.0 - ani->h/2;

	complex lastpos = plr->pos;
	plr->pos += (creal(delta)*xfac + cimag(delta)*yfac*I)*speed;
	complex realdir = plr->pos - lastpos;

	if(cabs(realdir)) {
		plr->lastmovedir = realdir / cabs(realdir);
	}
}

void player_draw(Player* plr) {
	// FIXME: death animation?
	if(plr->deathtime > global.frames)
		return;

	draw_enemies(plr->slaves);

	glPushMatrix();
		glTranslatef(creal(plr->pos), cimag(plr->pos), 0);

		if(plr->focus) {
			glPushMatrix();
				glRotatef(global.frames*10, 0, 0, 1);
				glScalef(1, 1, 1);
				glColor4f(1, 1, 1, 0.2 * (clamp(plr->focus, 0, 15) / 15.0));
				draw_texture(0, 0, "fairy_circle");
				glColor4f(1,1,1,1);
			glPopMatrix();
		}


		int clr_changed = 0;

		if(global.frames - abs(plr->recovery) < 0 && (global.frames/8)&1) {
			glColor4f(0.4,0.4,1,0.9);
			clr_changed = 1;
		}

		aniplayer_play(&plr->ani,0,0);

		if(clr_changed)
			glColor3f(1,1,1);

		if(plr->focus) {
			glPushMatrix();
				glColor4f(1, 1, 1, plr->focus / 30.0);
				glRotatef(global.frames, 0, 0, -1);
				draw_texture(0, 0, "focus");
				glColor4f(1, 1, 1, 1);
			glPopMatrix();
		}

	glPopMatrix();
}

static void player_fail_spell(Player *plr) {
	if(	!global.boss ||
		!global.boss->current ||
		global.boss->current->finished ||
		global.boss->current->failtime ||
		global.boss->current->starttime >= global.frames ||
		global.stage->type == STAGE_SPELL
	) {
		return;
	}

	global.boss->current->failtime = global.frames;

	if(global.boss->current->type == AT_ExtraSpell) {
		boss_finish_current_attack(global.boss);
	}
}

bool player_should_shoot(Player *plr, bool extra) {
	return (plr->inputflags & INFLAG_SHOT) &&
			(!extra || (global.frames - plr->recovery >= 0 && plr->deathtime >= -1));
}

void player_logic(Player* plr) {
	process_enemies(&plr->slaves);
	aniplayer_update(&plr->ani);
	if(plr->deathtime < -1) {
		plr->deathtime++;
		plr->pos -= I;
		return;
	}

	plr->focus = approach(plr->focus, (plr->inputflags & INFLAG_FOCUS) ? 30 : 0, 1);

	if(plr->mode->procs.think) {
		plr->mode->procs.think(plr);
	}

	if(player_should_shoot(plr, false)) {
		plr->mode->procs.shot(plr);
	}

	if(global.frames == plr->deathtime)
		player_realdeath(plr);

	if(global.frames - plr->recovery < 0) {
		Enemy *en;
		for(en = global.enemies; en; en = en->next)
			if(!en->unbombable && en->hp > ENEMY_IMMUNE)
				en->hp -= 300;

		if(global.boss) {
			boss_damage(global.boss, 30);
		}

		stage_clear_hazards(false);
		player_fail_spell(plr);
	}
}

bool player_bomb(Player *plr) {
	if(global.boss && global.boss->current && global.boss->current->type == AT_ExtraSpell)
		return false;

	if(global.frames - plr->recovery >= 0 && (plr->bombs > 0 || plr->iddqd) && global.frames - plr->respawntime >= 60) {
		player_fail_spell(plr);
		stage_clear_hazards(false);
		plr->mode->procs.bomb(plr);
		plr->bombs--;

		if(plr->deathtime > 0) {
			plr->deathtime = -1;

			if(plr->bombs)
				plr->bombs--;
		}

		if(plr->bombs < 0) {
			plr->bombs = 0;
		}

		plr->recovery = global.frames + BOMB_RECOVERY;

		return true;
	}

	return false;
}

void player_realdeath(Player *plr) {
	plr->deathtime = -DEATH_DELAY-1;
	plr->respawntime = global.frames;
	plr->inputflags &= ~INFLAGS_MOVE;

	complex death_origin = plr->pos;

	plr->pos = VIEWPORT_W/2 + VIEWPORT_H*I+30.0*I;
	plr->recovery = -(global.frames + DEATH_DELAY + 150);
	stage_clear_hazards(false);

	if(plr->iddqd)
		return;

	player_fail_spell(plr);

	if(global.stage->type != STAGE_SPELL && global.boss && global.boss->current && global.boss->current->type == AT_ExtraSpell) {
		// deaths in extra spells "don't count"
		return;
	}

	int drop = max(2, (plr->power * 0.15) / POWER_VALUE);
	spawn_items(death_origin, Power, drop, NULL);

	player_set_power(plr, plr->power * 0.7,true);
	plr->bombs = PLR_START_BOMBS;
	plr->bomb_fragments = 0;

	if(plr->lives-- == 0 && global.replaymode != REPLAY_PLAY) {
		stage_gameover();

		if(plr->lives > 0) {
			// game continued
			spawn_items(death_origin, Power, (int)ceil(PLR_MAX_POWER/(double)POWER_VALUE), NULL);
		}
	}
}

void player_death(Player *plr) {
	if(plr->deathtime == -1 && global.frames - abs(plr->recovery) > 0) {
		play_sound("death");
		int i;
		for(i = 0; i < 20; i++) {
			tsrand_fill(2);
			create_particle2c("flare", plr->pos, 0, Shrink, timeout_linear, 40, (3+afrand(0)*7)*cexp(I*tsrand_a(1)))->type=PlrProj;
		}
		create_particle2c("blast", plr->pos, rgba(1,0.3,0.3,0.5), GrowFadeAdd, timeout, 35, 2.4)->type=PlrProj;
		plr->deathtime = global.frames + DEATHBOMB_TIME;
	}
}

static PlrInputFlag key_to_inflag(KeyIndex key) {
	switch(key) {
		case KEY_UP:    return INFLAG_UP;    break;
		case KEY_DOWN:  return INFLAG_DOWN;  break;
		case KEY_LEFT:  return INFLAG_LEFT;  break;
		case KEY_RIGHT: return INFLAG_RIGHT; break;
		case KEY_FOCUS: return INFLAG_FOCUS; break;
		case KEY_SHOT:  return INFLAG_SHOT;  break;
		case KEY_SKIP:  return INFLAG_SKIP;  break;
		default:        return 0;
	}
}

bool player_updateinputflags(Player *plr, PlrInputFlag flags) {
	if(flags == plr->inputflags) {
		return false;
	}

	plr->inputflags = flags;
	return true;
}

bool player_updateinputflags_moveonly(Player *plr, PlrInputFlag flags) {
	return player_updateinputflags(plr, (flags & INFLAGS_MOVE) | (plr->inputflags & ~INFLAGS_MOVE));
}

bool player_setinputflag(Player *plr, KeyIndex key, bool mode) {
	PlrInputFlag newflags = plr->inputflags;
	PlrInputFlag keyflag = key_to_inflag(key);

	if(!keyflag) {
		return false;
	}

	if(mode) {
		newflags |= keyflag;
	} else {
		newflags &= ~keyflag;
	}

	return player_updateinputflags(plr, newflags);
}

static bool player_set_axis(int *aptr, uint16_t value) {
	int16_t new = (int16_t)value;

	if(*aptr == new) {
		return false;
	}

	*aptr = new;
	return true;
}

bool player_event(Player *plr, uint8_t type, uint16_t value) {
	bool useful = true;

	switch(type) {
		case EV_PRESS:
			if(global.dialog && (value == KEY_SHOT || value == KEY_BOMB)) {
				page_dialog(&global.dialog);
				break;
			}

			switch(value) {
				case KEY_BOMB:
					useful = player_bomb(plr);
					break;

				case KEY_IDDQD:
					plr->iddqd = !plr->iddqd;
					break;

				case KEY_POWERUP:
					useful = player_set_power(plr, plr->power + 100, true);
					break;

				case KEY_POWERDOWN:
					useful = player_set_power(plr, plr->power - 100, true);
					break;

				default:
					useful = player_setinputflag(plr, value, true);
					break;
			}
			break;

		case EV_RELEASE:
			player_setinputflag(plr, value, false);
			break;

		case EV_AXIS_LR:
			useful = player_set_axis(&plr->axis_lr, value);
			break;

		case EV_AXIS_UD:
			useful = player_set_axis(&plr->axis_ud, value);
			break;

		case EV_INFLAGS:
			useful = player_updateinputflags(plr, value);
			break;

		default:
			log_warn("Can not handle event: [%i:%02x:%04x]", global.frames, type, value);
			useful = false;
			break;
	}

	return useful;
}

bool player_event_with_replay(Player *plr, uint8_t type, uint16_t value) {
	assert(global.replaymode == REPLAY_RECORD);

	if(player_event(plr, type, value)) {
		replay_stage_event(global.replay_stage, global.frames, type, value);
		return true;
	} else {
		log_debug("Useless event discarded: [%i:%02x:%04x]", global.frames, type, value);
	}

	return false;
}

// free-axis movement
bool player_applymovement_gamepad(Player *plr) {
	if(!plr->axis_lr && !plr->axis_ud) {
		if(plr->gamepadmove) {
			plr->gamepadmove = false;
			plr->inputflags &= ~INFLAGS_MOVE;
		}
		return false;
	}

	complex direction = (plr->axis_lr + plr->axis_ud*I) / (double)GAMEPAD_AXIS_MAX;
	if(cabs(direction) > 1)
		direction /= cabs(direction);

	int sr = sign(creal(direction));
	int si = sign(cimag(direction));

	player_updateinputflags_moveonly(plr,
		(INFLAG_UP    * (si == -1)) |
		(INFLAG_DOWN  * (si ==  1)) |
		(INFLAG_LEFT  * (sr == -1)) |
		(INFLAG_RIGHT * (sr ==  1))
	);

	if(direction) {
		plr->gamepadmove = true;
		player_move(&global.plr, direction);
	}

	return true;
}

static void player_ani_moving(Player *plr, bool moving, bool dir) {
	plr->ani.stdrow = !moving;
	plr->ani.mirrored = dir;
}

void player_applymovement(Player *plr) {
	if(plr->deathtime < -1)
		return;

	bool gamepad = player_applymovement_gamepad(plr);
	player_ani_moving(plr,false,false);

	int up		=	plr->inputflags & INFLAG_UP,
		down	=	plr->inputflags & INFLAG_DOWN,
		left	=	plr->inputflags & INFLAG_LEFT,
		right	=	plr->inputflags & INFLAG_RIGHT;

	if(left && !right) {
		player_ani_moving(plr,true,true);
	} else if(right && !left) {
		player_ani_moving(plr,true,false);
	}

	if(gamepad)
		return;

	complex direction = 0;

	if(up)		direction -= 1.0*I;
	if(down)	direction += 1.0*I;
	if(left)	direction -= 1;
	if(right)	direction += 1;

	if(cabs(direction))
		direction /= cabs(direction);

	if(direction)
		player_move(&global.plr, direction);
}

void player_fix_input(Player *plr) {
	// correct input state to account for any events we might have missed,
	// usually because the pause menu ate them up

	PlrInputFlag newflags = plr->inputflags;

	for(KeyIndex key = KEYIDX_FIRST; key <= KEYIDX_LAST; ++key) {
		int flag = key_to_inflag(key);

		if(flag && !(plr->gamepadmove && (flag & INFLAGS_MOVE))) {
			bool flagset = plr->inputflags & flag;
			bool keyheld = gamekeypressed(key);

			if(flagset && !keyheld) {
				newflags &= ~flag;
			} else if(!flagset && keyheld) {
				newflags |= flag;
			}
		}
	}

	if(newflags != plr->inputflags) {
		player_event_with_replay(plr, EV_INFLAGS, newflags);
	}

	if(config_get_int(CONFIG_GAMEPAD_AXIS_FREE)) {
		int axis_lr = gamepad_get_player_axis_value(PLRAXIS_LR);
		int axis_ud = gamepad_get_player_axis_value(PLRAXIS_UD);

		if(plr->axis_lr != axis_lr) {
			player_event_with_replay(plr, EV_AXIS_LR, axis_lr);
		}

		if(plr->axis_ud != axis_ud) {
			player_event_with_replay(plr, EV_AXIS_UD, axis_ud);
		}
	}
}

void player_graze(Player *plr, complex pos, int pts) {
	plr->graze++;

	player_add_points(&global.plr, pts);
	play_sound("graze");

	int i = 0; for(i = 0; i < 5; ++i) {
		tsrand_fill(3);
		create_particle2c("flare", pos, 0, Shrink, timeout_linear, 5 + 5 * afrand(2), (1+afrand(0)*5)*cexp(I*tsrand_a(1)))->type=PlrProj;
	}
}

static void player_add_fragments(Player *plr, int frags, int *pwhole, int *pfrags, int maxfrags, int maxwhole, const char *fragsnd, const char *upsnd) {
	if(*pwhole >= maxwhole) {
		return;
	}

	*pfrags += frags;
	int up = *pfrags / maxfrags;

	*pwhole += up;
	*pfrags %= maxfrags;

	if(up) {
		play_sound(upsnd);
	}

	if(frags) {
		// FIXME: when we have the extra life/bomb sounds,
		//        don't play this if upsnd was just played.
		play_sound(fragsnd);
	}

	if(*pwhole > maxwhole) {
		*pwhole = maxwhole;
	}
}

void player_add_life_fragments(Player *plr, int frags) {
	player_add_fragments(plr, frags, &plr->lives, &plr->life_fragments, PLR_MAX_LIFE_FRAGMENTS, PLR_MAX_LIVES,
		"item_generic", // FIXME: replacement needed
		"extra_life"
	);
}

void player_add_bomb_fragments(Player *plr, int frags) {
	player_add_fragments(plr, frags, &plr->bombs, &plr->bomb_fragments, PLR_MAX_BOMB_FRAGMENTS, PLR_MAX_BOMBS,
		"item_generic",  // FIXME: replacement needed
		"extra_bomb"
	);
}

void player_add_lives(Player *plr, int lives) {
	player_add_life_fragments(plr, PLR_MAX_LIFE_FRAGMENTS);
}

void player_add_bombs(Player *plr, int bombs) {
	player_add_bomb_fragments(plr, PLR_MAX_BOMB_FRAGMENTS);
}


static void try_spawn_bonus_item(Player *plr, ItemType type, unsigned int oldpoints, unsigned int reqpoints) {
	int items = plr->points / reqpoints - oldpoints / reqpoints;

	if(items > 0) {
		complex p = creal(plr->pos);
		create_item(p, -5*I, type);
		spawn_items(p, type, --items, NULL);
	}
}

void player_add_points(Player *plr, unsigned int points) {
	unsigned int old = plr->points;
	plr->points += points;

	if(global.stage->type != STAGE_SPELL) {
		try_spawn_bonus_item(plr, LifeFrag, old, PLR_SCORE_PER_LIFE_FRAG);
		try_spawn_bonus_item(plr, BombFrag, old, PLR_SCORE_PER_BOMB_FRAG);
	}
}

void player_preload(void) {
	const int flags = RESF_DEFAULT;

	preload_resources(RES_TEXTURE, flags,
		"focus",
		"fairy_circle",
	NULL);

	preload_resources(RES_SFX, flags | RESF_OPTIONAL,
		"graze",
		"death",
		"generic_shot",
		"full_power",
		"extra_life",
		"extra_bomb",
	NULL);
}
