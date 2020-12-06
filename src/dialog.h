/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_dialog_h
#define IGUARD_dialog_h

#include "taisei.h"

#include "color.h"
#include "resource/sprite.h"
#include "coroutine.h"

typedef enum DialogSide {
	DIALOG_SIDE_RIGHT,
	DIALOG_SIDE_LEFT,
} DialogSide;

typedef enum DialogState {
	DIALOG_STATE_IDLE,
	DIALOG_STATE_WAITING_FOR_SKIP,
	DIALOG_STATE_FADEOUT,
} DialogState;

typedef struct DialogActor {
	LIST_INTERFACE(struct DialogActor);

	const char *name;
	const char *variant;
	const char *face;

	Sprite composite;

	float opacity;
	float target_opacity;

	float focus;
	float target_focus;

	Color speech_color;

	FloatOffset offset;
	DialogSide side;

	bool composite_dirty;
} DialogActor;

typedef struct DialogTextBuffer {
	const char *text;
	Color color;
	float opacity;
} DialogTextBuffer;

typedef struct Dialog {
	LIST_ANCHOR(DialogActor) actors;

	struct {
		DialogTextBuffer buffers[2];
		DialogTextBuffer *current;
		DialogTextBuffer *fading_out;
	} text;

	struct {
		const char *name;
		const char *text;
		bool active;
		int timeout;
		struct {
			float opacity;
		} box, box_text;
	} title;

	Color title_color;

	COEVENTS_ARRAY(
		skip_requested,
		fadeout_began,
		fadeout_ended
	) events;

	DialogState state;

	float opacity;
} Dialog;

typedef struct DialogMessageParams {
	const char *text;
	DialogActor *actor;
	int wait_timeout;
	bool implicit_wait;
	bool wait_skippable;
} DialogMessageParams;

void dialog_init(Dialog *d)
	attr_nonnull_all;

void dialog_deinit(Dialog *d)
	attr_nonnull_all;

void dialog_add_actor(Dialog *d, DialogActor *a, const char *name, DialogSide side)
	attr_nonnull_all;

void dialog_actor_set_face(DialogActor *a, const char *face)
	attr_nonnull_all;

void dialog_actor_set_variant(DialogActor *a, const char *variant)
	attr_nonnull(1);

attr_nonnull_all INLINE void dialog_actor_show(DialogActor *a) { a->target_opacity = 1; }
attr_nonnull_all INLINE void dialog_actor_hide(DialogActor *a) { a->target_opacity = 0; }

void dialog_skippable_wait(Dialog *d, int timeout)
	attr_nonnull_all;

int dialog_util_estimate_wait_timeout_from_text(const char *text)
	attr_nonnull_all;

void dialog_message(Dialog *d, DialogActor *actor, const char *text)
	attr_nonnull_all;

void dialog_message_unskippable(Dialog *d, DialogActor *actor, const char *text, int delay)
	attr_nonnull_all;

void dialog_message_ex(Dialog *d, const DialogMessageParams *params)
	attr_nonnull_all;

void dialog_focus_actor(Dialog *d, DialogActor *actor)
	attr_nonnull_all;

void dialog_end(Dialog *d)
	attr_nonnull_all;

void dialog_update(Dialog *d)
	attr_nonnull_all;

void dialog_draw(Dialog *d);

bool dialog_page(Dialog *d)
	attr_nonnull_all;

void dialog_show_title(Dialog *d, DialogActor *actor, char *name, char *title)
	attr_nonnull_all;

bool dialog_is_active(Dialog *d);

void dialog_preload(void);

#include "dialog/dialog_interface.h"

#endif // IGUARD_dialog_h
