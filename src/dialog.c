/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "dialog.h"
#include "global.h"
#include "portrait.h"

void dialog_init(Dialog *d) {
	memset(d, 0, sizeof(*d));
	d->state = DIALOG_STATE_IDLE;
	d->text.current = &d->text.buffers[0];
	d->text.fading_out = &d->text.buffers[1];
	COEVENT_INIT_ARRAY(d->events);
}

void dialog_deinit(Dialog *d) {
	COEVENT_CANCEL_ARRAY(d->events);

	for(DialogActor *a = d->actors.first; a; a = a->next) {
		if(a->composite.tex) {
			r_texture_destroy(a->composite.tex);
		}
	}
}

void dialog_add_actor(Dialog *d, DialogActor *a, const char *name, DialogSide side) {
	memset(a, 0, sizeof(*a));
	a->name = name;
	a->face = "normal";
	a->side = side;
	a->target_opacity = 1;
	a->composite_dirty = true;

	if(side == DIALOG_SIDE_RIGHT) {
		a->speech_color = *RGB(0.6, 0.6, 1.0);
	} else {
		a->speech_color = *RGB(1.0, 1.0, 1.0);
	}

	alist_append(&d->actors, a);
}

void dialog_actor_set_face(DialogActor *a, const char *face) {
	log_debug("[%s] %s --> %s", a->name, a->face, face);
	if(a->face != face) {
		a->face = face;
		a->composite_dirty = true;
	}
}

void dialog_actor_set_variant(DialogActor *a, const char *variant) {
	log_debug("[%s] %s --> %s", a->name, a->variant, variant);
	if(a->variant != variant) {
		a->variant = variant;
		a->composite_dirty = true;
	}
}

void dialog_update(Dialog *d) {
	if(d->text.current->text) {
		fapproach_p(&d->text.current->opacity, 1, 1/120.0f);
	} else {
		d->text.current->opacity = 0;
	}

	if(d->text.fading_out->text) {
		fapproach_p(&d->text.fading_out->opacity, 0, 1/60.0f);
	} else {
		d->text.fading_out->opacity = 0;
	}

	if(
		d->state == DIALOG_STATE_FADEOUT ||
		(
			d->text.current->opacity == 0 &&
			d->text.fading_out->opacity < 0.25
		)
	) {
		fapproach_asymptotic_p(&d->opacity, 0, 0.1, 1e-3);
	} else {
		fapproach_asymptotic_p(&d->opacity, 1, 0.05, 1e-3);
	}

	const float offset_per_actor = 32;
	float target_offsets[2] = { 0 };

	for(DialogActor *a = d->actors.last; a; a = a->prev) {
		fapproach_asymptotic_p(&a->offset.x, target_offsets[a->side], 0.10, 1e-3);
		target_offsets[a->side] += offset_per_actor;

		if(d->state == DIALOG_STATE_FADEOUT) {
			fapproach_asymptotic_p(&a->opacity, 0, 0.12, 1e-3);
		} else {
			fapproach_asymptotic_p(&a->opacity, a->target_opacity, 0.04, 1e-3);
		}

		fapproach_asymptotic_p(&a->focus, a->target_focus, 0.12, 1e-3);
	}

	if (d->title.active) {
		if(d->title.timeout > 0) {
			fapproach_asymptotic_p(&d->title.box.opacity, 1, 0.05, 1e-3);
			if(d->title.box.opacity >= 0.9 ) {
				fapproach_asymptotic_p(&d->title.box_text.opacity, 1, 0.05, 1e-3);
			}
			d->title.timeout--;
		}
		if(d->title.timeout == 0 || d->state == DIALOG_STATE_FADEOUT) {
			fapproach_asymptotic_p(&d->title.box.opacity, 0, 0.1, 1e-3);
			fapproach_asymptotic_p(&d->title.box_text.opacity, 0, 0.1, 1e-3);
			d->title.box_text.opacity = 0;
			d->title.timeout = 0;
		}
	} else if (d->title.timeout == 0) {
		d->title.active = false;
	}
}

void dialog_skippable_wait(Dialog *d, int timeout) {
	CoEventSnapshot snap = coevent_snapshot(&d->events.skip_requested);

	assert(d->state == DIALOG_STATE_IDLE);
	d->state = DIALOG_STATE_WAITING_FOR_SKIP;

	while(timeout > 0) {
		dialog_update(d);

		--timeout;
		YIELD;

		if(coevent_poll(&d->events.skip_requested, &snap) != CO_EVENT_PENDING) {
			log_debug("Skipped with %i remaining", timeout);
			break;
		}
	}

	assert(d->state == DIALOG_STATE_WAITING_FOR_SKIP);
	d->state = DIALOG_STATE_IDLE;

	if(timeout == 0) {
		log_debug("Timed out");
	}
}

int dialog_util_estimate_wait_timeout_from_text(const char *text) {
	return 1800;
}

static void dialog_set_text(Dialog *d, const char *text, const Color *clr) {
	DialogTextBuffer *temp = d->text.current;
	d->text.current = d->text.fading_out;
	d->text.fading_out = temp;

	d->text.current->color = *clr;
	d->text.current->text = text;
}

void dialog_focus_actor(Dialog *d, DialogActor *actor) {
	for(DialogActor *a = d->actors.first; a; a = a->next) {
		a->target_focus = 0;
	}

	actor->target_focus = 1;

	// make focused actor drawn on top of everyone else
	alist_unlink(&d->actors, actor);
	alist_append(&d->actors, actor);
}

void dialog_message_ex(Dialog *d, const DialogMessageParams *params) {
	assume(params->actor != NULL);
	assume(params->text != NULL);


	dialog_set_text(d, params->text, &params->actor->speech_color);
	dialog_focus_actor(d, params->actor);

	if(params->implicit_wait) {
		assume(params->wait_timeout > 0);

		if(params->wait_skippable) {
			dialog_skippable_wait(d, params->wait_timeout);
		} else {
			WAIT(params->wait_timeout);
		}
	}
}

static void _dialog_message(Dialog *d, DialogActor *actor, const char *text, bool skippable, int delay) {
	DialogMessageParams p = { 0 };
	p.actor = actor;
	p.text = text;
	p.implicit_wait = true;
	p.wait_skippable = skippable;
	p.wait_timeout = delay;
	dialog_message_ex(d, &p);
}

void dialog_message(Dialog *d, DialogActor *actor, const char *text) {
	_dialog_message(d, actor, text, true, dialog_util_estimate_wait_timeout_from_text(text));
}

void dialog_message_unskippable(Dialog *d, DialogActor *actor, const char *text, int delay) {
	_dialog_message(d, actor, text, false, delay);
}

void dialog_end(Dialog *d) {
	d->state = DIALOG_STATE_FADEOUT;
	coevent_signal(&d->events.fadeout_began);

	for(DialogActor *a = d->actors.first; a; a = a->next) {
		a->target_opacity = 0;
	}

	wait_for_fadeout: {
		if(d->opacity > 0) {
			YIELD;
			goto wait_for_fadeout;
		}

		for(DialogActor *a = d->actors.first; a; a = a->next) {
			if(a->opacity > 0) {
				YIELD;
				goto wait_for_fadeout;
			}
		}
	}

	coevent_signal(&d->events.fadeout_ended);
	dialog_deinit(d);
}

static void dialog_actor_update_composite(DialogActor *a) {
	assume(a->name != NULL);
	assume(a->face != NULL);

	if(!a->composite_dirty) {
		return;
	}

	log_debug("%s (%p) is dirty; face=%s; variant=%s", a->name, (void*)a, a->face, a->variant);

	if(a->composite.tex != NULL) {
		log_debug("destroyed texture at %p", (void*)a->composite.tex);
		r_texture_destroy(a->composite.tex);
	}

	portrait_render_byname(a->name, a->variant, a->face, &a->composite);
	log_debug("created texture at %p", (void*)a->composite.tex);
	a->composite_dirty = false;
}

void dialog_draw(Dialog *dialog) {
	if(dialog == NULL) {
		return;
	}

	for(DialogActor *a = dialog->actors.first; a; a = a->next) {
		dialog_actor_update_composite(a);
	}

	r_state_push();
	r_state_push();
	r_shader("sprite_default");

	r_mat_mv_push();
	r_mat_mv_translate(VIEWPORT_X, 0, 0);

	const double dialog_width = VIEWPORT_W * 1.2;

	r_mat_mv_push();
	r_mat_mv_translate(dialog_width/2.0, 64, 0);

	Color clr = { 0 };

	for(DialogActor *a = dialog->actors.first; a; a = a->next) {
		if(a->opacity <= 0) {
			continue;
		}

		dialog_actor_update_composite(a);
		Sprite *portrait = &a->composite;
		assume(portrait->tex != NULL);

		float portrait_w = sprite_padded_width(portrait);
		float portrait_h = sprite_padded_height(portrait);

		r_mat_mv_push();

		if(a->side == DIALOG_SIDE_LEFT) {
			r_cull(CULL_FRONT);
			r_mat_mv_scale(-1, 1, 1);
		} else {
			r_cull(CULL_BACK);
		}

		if(a->opacity < 1) {
			r_mat_mv_translate(120 * (1 - a->opacity), 0, 0);
		}

		float ofs = 10 * (1 - a->focus);
		r_mat_mv_translate(ofs, ofs, 0);
		float brightness = 0.5 + 0.5 * a->focus;
		clr.r = clr.g = clr.b = brightness;
		clr.a = 1;

		color_mul_scalar(&clr, a->opacity);

		r_flush_sprites();
		r_draw_sprite(&(SpriteParams) {
			.blend = BLEND_PREMUL_ALPHA,
			.color = &clr,
			.pos.x = (dialog_width - portrait_w) / 2 + 32 + a->offset.x,
			.pos.y = VIEWPORT_H - portrait_h / 2 + a->offset.y,
			.sprite_ptr = portrait,
		});

		r_mat_mv_pop();
	}

	r_mat_mv_pop();
	r_state_pop();

	FloatRect dialog_bg_rect = {
		.extent = { VIEWPORT_W-40, 110 },
		.offset = { VIEWPORT_W/2, VIEWPORT_H-55 },
	};

	r_mat_mv_push();

	if(dialog->opacity < 1) {
		r_mat_mv_translate(0, 100 * (1 - dialog->opacity), 0);
	}
	r_color4(0, 0, 0, 0.8 * dialog->opacity);
	r_mat_mv_push();
	r_mat_mv_translate(dialog_bg_rect.x, dialog_bg_rect.y, 0);
	r_mat_mv_scale(dialog_bg_rect.w, dialog_bg_rect.h, 1);
	r_shader_standard_notex();
	r_draw_quad();
	r_mat_mv_pop();


	Font *font = res_font("standard");

	r_mat_tex_push();

	dialog_bg_rect.w = VIEWPORT_W * 0.86;
	dialog_bg_rect.x -= dialog_bg_rect.w * 0.5;
	dialog_bg_rect.y -= dialog_bg_rect.h * 0.5;


	if(dialog->text.fading_out->opacity > 0) {
		clr = dialog->text.fading_out->color;
		color_mul_scalar(&clr, dialog->opacity);

		text_draw_wrapped(dialog->text.fading_out->text, dialog_bg_rect.w, &(TextParams) {
			.shader = "text_dialog",
			.aux_textures = { res_texture("cell_noise") },
			.shader_params = &(ShaderCustomParams) {{ dialog->opacity * (1.0 - (0.2 + 0.8 * (1 - dialog->text.fading_out->opacity))), 1 }},
			.color = &clr,
			.pos = { VIEWPORT_W/2, VIEWPORT_H-110 + font_get_lineskip(font) },
			.align = ALIGN_CENTER,
			.font_ptr = font,
			.overlay_projection = &dialog_bg_rect,
		});
	}

	if(dialog->text.current->opacity > 0) {
		clr = dialog->text.current->color;
		color_mul_scalar(&clr, dialog->opacity);

		text_draw_wrapped(dialog->text.current->text, dialog_bg_rect.w, &(TextParams) {
			.shader = "text_dialog",
			.aux_textures = { res_texture("cell_noise") },
			.shader_params = &(ShaderCustomParams) {{ dialog->opacity * dialog->text.current->opacity, 0 }},
			.color = &clr,
			.pos = { VIEWPORT_W/2, VIEWPORT_H-110 + font_get_lineskip(font) },
			.align = ALIGN_CENTER,
			.font_ptr = font,
			.overlay_projection = &dialog_bg_rect,
		});
	}

	if(dialog->title.active) {
		FloatRect title_bg_rect = {
			.extent = { VIEWPORT_W-300, 60 },
			.offset = { VIEWPORT_W-125, VIEWPORT_H-170 },
		};

		r_mat_mv_push();
		if(dialog->title.box.opacity < 1) {
			r_mat_mv_translate(0, 100 * (1 - dialog->title.box.opacity), 0);
		}
		r_color4(0, 0, 0, 0.8 * dialog->title.box.opacity);
		r_mat_mv_translate(title_bg_rect.x, title_bg_rect.y, 0);
		r_mat_mv_scale(title_bg_rect.w, title_bg_rect.h, 1);
		r_shader_standard_notex();
		r_draw_quad();
		r_mat_mv_pop();

		title_bg_rect.w = VIEWPORT_W * 0.96;
		title_bg_rect.x -= title_bg_rect.w * 0.6;
		title_bg_rect.y -= title_bg_rect.h * 0.6;

		clr = dialog->text.current->color;
		color_mul_scalar(&clr, dialog->title.box_text.opacity);

		text_draw(dialog->title.name, &(TextParams) {
			.shader = "text_default",
			.color = &clr,
			.pos = {
				title_bg_rect.y-10,
				title_bg_rect.x+300
			},
			.align = ALIGN_CENTER,
			.font_ptr = font,
		});

	}


	r_mat_tex_pop();
	r_mat_mv_pop();
	r_mat_mv_pop();
	r_state_pop();
}

bool dialog_page(Dialog *d) {
	if(d->state == DIALOG_STATE_WAITING_FOR_SKIP) {
		coevent_signal(&d->events.skip_requested);
		return true;
	}

	return false;
}

bool dialog_is_active(Dialog *d) {
	return d && d->state != DIALOG_STATE_FADEOUT;
}

void dialog_preload(void) {
	preload_resource(RES_SHADER_PROGRAM, "text_dialog", RESF_DEFAULT);
}

void dialog_show_title(Dialog *dialog, DialogActor *actor, char *name, char *title) {
	dialog->title.name = name;
	dialog->title.text = title;
	dialog->title.active = true;
	dialog->title.timeout = 360;

	log_debug("%s - %s", name, title);
}
