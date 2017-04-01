/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#include "events.h"
#include "config.h"
#include "global.h"
#include "video.h"
#include "gamepad.h"
#include "transition.h"

void handle_events(EventHandler handler, EventFlags flags, void *arg) {
	SDL_Event event;

	bool kbd 	= flags & EF_Keyboard;
	bool text	= flags & EF_Text;
	bool menu	= flags & EF_Menu;
	bool game	= flags & EF_Game;

	// TODO: rewrite text input handling to properly support multibyte characters and IMEs

	if(text) {
		if(!SDL_IsTextInputActive()) {
			SDL_StartTextInput();
		}
	} else {
		if(SDL_IsTextInputActive()) {
			SDL_StopTextInput();
		}
	}

	while(SDL_PollEvent(&event)) {
		if(resource_sdl_event(&event)) {
			continue;
		}

		SDL_Scancode scan = event.key.keysym.scancode;
		SDL_Keymod mod = static_cast<SDL_Keymod>(event.key.keysym.mod);
		bool repeat = event.key.repeat;

		switch(event.type) {
			case SDL_KEYDOWN:
				if(text) {
					if(scan == SDL_SCANCODE_ESCAPE)
						handler(E_CancelText, 0, arg);
					else if(scan == SDL_SCANCODE_RETURN)
						handler(E_SubmitText, 0, arg);
					else if(scan == SDL_SCANCODE_BACKSPACE)
						handler(E_CharErased, 0, arg);
				} else if(!repeat) {
					if(scan == config_get_int(CONFIG_KEY_SCREENSHOT)) {
						video_take_screenshot();
						break;
					}

					if((scan == SDL_SCANCODE_RETURN && (mod & KMOD_ALT)) || scan == config_get_int(CONFIG_KEY_FULLSCREEN)) {
						config_set_int(CONFIG_FULLSCREEN, !config_get_int(CONFIG_FULLSCREEN));
						break;
					}
				}

				if(kbd) {
					handler(E_KeyDown, scan, arg);
				}

				if(menu && (!repeat || transition.state == TRANS_IDLE)) {
					if(scan == config_get_int(CONFIG_KEY_DOWN) || scan == SDL_SCANCODE_DOWN) {
						handler(E_CursorDown, 0, arg);
					} else if(scan == config_get_int(CONFIG_KEY_UP) || scan == SDL_SCANCODE_UP) {
						handler(E_CursorUp, 0, arg);
					} else if(scan == config_get_int(CONFIG_KEY_RIGHT) || scan == SDL_SCANCODE_RIGHT) {
						handler(E_CursorRight, 0, arg);
					} else if(scan == config_get_int(CONFIG_KEY_LEFT) || scan == SDL_SCANCODE_LEFT) {
						handler(E_CursorLeft, 0, arg);
					} else if(scan == config_get_int(CONFIG_KEY_SHOT) || scan == SDL_SCANCODE_RETURN) {
						handler(E_MenuAccept, 0, arg);
					} else if(scan == config_get_int(CONFIG_KEY_BOMB) || scan == SDL_SCANCODE_ESCAPE) {
						handler(E_MenuAbort, 0, arg);
					}
				}

				if(game && !repeat) {
					if(scan == config_get_int(CONFIG_KEY_PAUSE) || scan == SDL_SCANCODE_ESCAPE) {
						handler(E_Pause, 0, arg);
					} else {
						int key = config_key_from_scancode(scan);
						if(key >= 0)
							handler(E_PlrKeyDown, key, arg);
					}
				}

				break;

			case SDL_KEYUP:
				if(kbd) {
					handler(E_KeyUp, scan, arg);
				}

				if(game && !repeat) {
					int key = config_key_from_scancode(scan);
					if(key >= 0)
						handler(E_PlrKeyUp, key, arg);
				}

				break;

			case SDL_TEXTINPUT: {
				char *c;

				for(c = event.text.text; *c; ++c) {
					handler(E_CharTyped, *c, arg);
				}

				break;
			}

			case SDL_WINDOWEVENT:
				switch(event.window.event) {
					case SDL_WINDOWEVENT_RESIZED:
						video_resize(event.window.data1, event.window.data2);
						break;

					case SDL_WINDOWEVENT_FOCUS_LOST:
						if(game) {
							handler(E_Pause, 0, arg);
						}
						break;
				}
				break;

			case SDL_QUIT:
				exit(0);
				break;

			default:
				gamepad_event(&event, handler, flags, arg);
				break;
		}
	}
}

// Inputdevice-agnostic method of checking whether a game control is pressed.
// ALWAYS use this instead of SDL_GetKeyState if you need it.
bool gamekeypressed(KeyIndex key) {
	return SDL_GetKeyboardState(NULL)[config_get_int(KEYIDX_TO_CFGIDX(key))] || gamepad_gamekeypressed(key);
}
