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
#include <stdbool.h>
#include "events.h"
#include "config.h"

enum {
	AXISVAL_LEFT  = -1,
	AXISVAL_RIGHT =  1,

	AXISVAL_UP    = -1,
	AXISVAL_DOWN  =  1,

	AXISVAL_NULL  = 0
};

typedef enum {
	PLRAXIS_LR, // aka X
	PLRAXIS_UD, // aka Y
} GamepadPlrAxis;

typedef enum GamepadEmulatedButton {
	GAMEPAD_EMULATED_BUTTON_INVALID = -1,
	GAMEPAD_EMULATED_BUTTON_TRIGGER_LEFT,
	GAMEPAD_EMULATED_BUTTON_TRIGGER_RIGHT,
	GAMEPAD_EMULATED_BUTTON_MAX,
} GamepadEmulatedButton;

typedef enum GamepadButton {
	// must match SDL_GameControllerButton
	GAMEPAD_BUTTON_INVALID = -1,
	GAMEPAD_BUTTON_A,
	GAMEPAD_BUTTON_B,
	GAMEPAD_BUTTON_X,
	GAMEPAD_BUTTON_Y,
	GAMEPAD_BUTTON_BACK,
	GAMEPAD_BUTTON_GUIDE,
	GAMEPAD_BUTTON_START,
	GAMEPAD_BUTTON_STICK_LEFT,
	GAMEPAD_BUTTON_STICK_RIGHT,
	GAMEPAD_BUTTON_SHOULDER_LEFT,
	GAMEPAD_BUTTON_SHOULDER_RIGHT,
	GAMEPAD_BUTTON_DPAD_UP,
	GAMEPAD_BUTTON_DPAD_DOWN,
	GAMEPAD_BUTTON_DPAD_LEFT,
	GAMEPAD_BUTTON_DPAD_RIGHT,
	GAMEPAD_BUTTON_MAX,

	GAMEPAD_BUTTON_EMULATED = 0x8000,

	GAMEPAD_BUTTON_TRIGGER_LEFT = GAMEPAD_EMULATED_BUTTON_TRIGGER_LEFT | GAMEPAD_BUTTON_EMULATED,
	GAMEPAD_BUTTON_TRIGGER_RIGHT = GAMEPAD_EMULATED_BUTTON_TRIGGER_RIGHT | GAMEPAD_BUTTON_EMULATED,
} GamepadButton;

typedef enum GamepadAxis {
	// must match SDL_GameControllerAxis
	GAMEPAD_AXIS_INVALID = -1,
	GAMEPAD_AXIS_LEFT_X,
	GAMEPAD_AXIS_LEFT_Y,
	GAMEPAD_AXIS_RIGHT_X,
	GAMEPAD_AXIS_RIGHT_Y,
	GAMEPAD_AXIS_TRIGGER_LEFT,
	GAMEPAD_AXIS_TRIGGER_RIGHT,
	GAMEPAD_AXIS_MAX
} GamepadAxis;

void gamepad_init(void);
void gamepad_shutdown(void);
void gamepad_restart(void);
void gamepad_event(SDL_Event*, EventHandler, EventFlags, void*);

int gamepad_device_count(void);
const char* gamepad_device_name(int);
void gamepad_device_guid(int num, char *guid_str, size_t guid_str_sz);
int gamepad_device_num_from_guid(const char *guid_str);
int gamepad_current_device_num(void);

bool gamepad_button_pressed(GamepadButton btn);
bool gamepad_game_key_pressed(KeyIndex key);

const char* gamepad_button_name(GamepadButton btn);
const char* gamepad_axis_name(GamepadAxis btn);

GamepadButton gamepad_button_from_name(const char *name);
GamepadAxis gamepad_axis_from_name(const char *name);

GamepadButton gamepad_button_from_axis(GamepadAxis axis);
GamepadButton gamepad_button_from_sdl_button(SDL_GameControllerButton btn);
SDL_GameControllerButton gamepad_button_to_sdl_button(GamepadButton btn);

GamepadAxis gamepad_axis_from_sdl_axis(SDL_GameControllerAxis axis);
SDL_GameControllerAxis gamepad_axis_to_sdl_axis(GamepadAxis axis);

int gamepad_axis_value(GamepadAxis paxis);
int gamepad_player_axis_value(GamepadPlrAxis paxis);

#define GAMEPAD_AXIS_MAX_VALUE 32767
#define GAMEPAD_AXIS_MIN_VALUE -32768
#define AXISVAL sign
