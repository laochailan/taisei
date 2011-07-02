/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef PLAYER_H
#define PLAYER_H

#include <complex.h>
#include "enemy.h"

#include "resource/animation.h"

enum {
	False = 0,
	True = 1
};

typedef enum {
	Youmu,
	Marisa
} Character;

typedef enum {
	YoumuOpposite,
	YoumuHoming,
	
	MarisaLaser = YoumuOpposite,
	MarisaBar = YoumuHoming
} ShotMode;

typedef struct {
	complex pos;
	short focus;
	short fire;
	short moving;	
	
	short dir;
	float power;
    
	int lifes;
	int bombs;
	
	int recovery;
	
	int deathtime;
    
    int continues;
    
	Character cha;
	ShotMode shot;
	Enemy *slaves;
	
	Animation *ani;
} Player;

void init_player(Player*, Character cha, ShotMode shot);

void player_draw(Player*);
void player_logic(Player*);

void plr_set_power(Player *plr, float npow);

void plr_bomb(Player*);
void plr_realdeath(Player*);
void plr_death(Player*);
#endif
