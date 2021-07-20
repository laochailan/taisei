/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "global.h"
#include "stagedraw.h"
#include "stagetext.h"
#include "video.h"
#include "resource/postprocess.h"
#include "entity.h"
#include "util/fbmgr.h"
#include "replay/struct.h"

#ifdef DEBUG
	#define GRAPHS_DEFAULT 1
	#define OBJPOOLSTATS_DEFAULT 0
#else
	#define GRAPHS_DEFAULT 0
	#define OBJPOOLSTATS_DEFAULT 0
#endif

#define SPELL_INTRO_DURATION 120
#define SPELL_INTRO_TIME_FACTOR 0.8

static struct {
	struct {
		ShaderProgram *shader;
		Font *font;

		struct {
			Color active;
			Color inactive;
			Color dark;
			Color label;
			Color label_power;
			Color label_value;
			Color label_graze;
			Color label_voltage;
		} color;
	} hud_text;

	struct {
		ShaderProgram *fxaa;
		ShaderProgram *copy_depth;
	} shaders;

	PostprocessShader *viewport_pp;
	FBPair fb_pairs[NUM_FBPAIRS];
	FBPair powersurge_fbpair;
	FBPair *current_postprocess_fbpair;

	ManagedFramebufferGroup *mfb_group;
	StageDrawEvents events;

	struct {
		float alpha;
		float target_alpha;
	} clear_screen;

	bool framerate_graphs;
	bool objpool_stats;

	#ifdef DEBUG
		Sprite dummy;
	#endif
} stagedraw = {
	.hud_text.color = {
		// NOTE: premultiplied alpha
		.active        = { 1.00, 1.00, 1.00, 1.00 },
		.inactive      = { 0.50, 0.50, 0.50, 0.70 },
		.dark          = { 0.30, 0.30, 0.30, 0.70 },
		.label         = { 1.00, 1.00, 1.00, 1.00 },
		.label_power   = { 1.00, 0.50, 0.50, 1.00 },
		.label_value   = { 0.30, 0.60, 1.00, 1.00 },
		.label_graze   = { 0.50, 1.00, 0.50, 1.00 },
		.label_voltage = { 0.80, 0.50, 1.00, 1.00 },
	}
};

static double fb_scale(void) {
	float vp_width, vp_height;
	video_get_viewport_size(&vp_width, &vp_height);
	return (double)vp_height / SCREEN_H;
}

static void set_fb_size(StageFBPair fb_id, int *w, int *h, float scale_worst, float scale_best) {
	double scale = fb_scale();

	if(fb_id == FBPAIR_FG_AUX || fb_id == FBPAIR_BG_AUX) {
		scale_worst *= 0.5;
	}

	int pp_qual = config_get_int(CONFIG_POSTPROCESS);

	// We might lerp between these in the future
	if(pp_qual < 2) {
		scale *= scale_worst;
	} else {
		scale *= scale_best;
	}

	log_debug("%u %f %f %f %i", fb_id, scale, scale_worst, scale_best, pp_qual);

	switch(fb_id) {
		case FBPAIR_BG:
			scale *= config_get_float(CONFIG_BG_QUALITY);
			// fallthrough

		default:
			scale *= config_get_float(CONFIG_FG_QUALITY);
			break;
	}

	scale = fmax(0.1, scale);
	*w = round(VIEWPORT_W * scale);
	*h = round(VIEWPORT_H * scale);
}

typedef struct StageFramebufferResizeParams {
	struct { float worst, best; } scale;
	StageFBPair scaling_base;
	int refs;
} StageFramebufferResizeParams;

static void stage_framebuffer_resize_strategy(void *userdata, IntExtent *out_dimensions, FloatRect *out_viewport) {
	StageFramebufferResizeParams *rp = userdata;
	set_fb_size(rp->scaling_base, &out_dimensions->w, &out_dimensions->h, rp->scale.worst, rp->scale.best);
	*out_viewport = (FloatRect) { 0, 0, out_dimensions->w, out_dimensions->h };
}

static void stage_framebuffer_resize_strategy_cleanup(void *userdata) {
	StageFramebufferResizeParams *rp = userdata;
	if(--rp->refs <= 0) {
		free(rp);
	}
}

static bool stage_draw_event(SDL_Event *e, void *arg) {
	if(!IS_TAISEI_EVENT(e->type)) {
		return false;
	}

	switch(TAISEI_EVENT(e->type)) {
		case TE_FRAME: {
			fapproach_p(&stagedraw.clear_screen.alpha, stagedraw.clear_screen.target_alpha, 0.01);
			break;
		}
	}

	return false;
}

static void stage_draw_fbpair_create(
	FBPair *pair,
	int num_attachments,
	FBAttachmentConfig *attachments,
	const StageFramebufferResizeParams *resize_params,
	const char *name
) {
	StageFramebufferResizeParams *rp = memdup(resize_params, sizeof(*resize_params));
	rp->refs = 2;

	FramebufferConfig fbconf = { 0 };
	fbconf.attachments = attachments;
	fbconf.num_attachments = num_attachments;
	fbconf.resize_strategy.resize_func = stage_framebuffer_resize_strategy;
	fbconf.resize_strategy.userdata = rp;
	fbconf.resize_strategy.cleanup_func = stage_framebuffer_resize_strategy_cleanup;
	fbmgr_group_fbpair_create(stagedraw.mfb_group, name, &fbconf, pair);
}

static void stage_draw_setup_framebuffers(void) {
	FBAttachmentConfig a[2], *a_color, *a_depth;
	memset(a, 0, sizeof(a));

	a_color = &a[0];
	a_depth = &a[1];

	a_color->attachment = FRAMEBUFFER_ATTACH_COLOR0;
	a_depth->attachment = FRAMEBUFFER_ATTACH_DEPTH;

	StageFramebufferResizeParams rp_fg =     { .scaling_base = FBPAIR_FG,     .scale.best = 1, .scale.worst = 1 };
	StageFramebufferResizeParams rp_fg_aux = { .scaling_base = FBPAIR_FG_AUX, .scale.best = 1, .scale.worst = 1 };
	StageFramebufferResizeParams rp_bg =     { .scaling_base = FBPAIR_BG,     .scale.best = 1, .scale.worst = 1 };
	StageFramebufferResizeParams rp_bg_aux = { .scaling_base = FBPAIR_BG_AUX, .scale.best = 1, .scale.worst = 1 };

	// Set up some parameters shared by all attachments
	TextureParams tex_common = {
		.filter.min = TEX_FILTER_LINEAR,
		.filter.mag = TEX_FILTER_LINEAR,
		.wrap.s = TEX_WRAP_MIRROR,
		.wrap.t = TEX_WRAP_MIRROR,
	};

	memcpy(&a_color->tex_params, &tex_common, sizeof(tex_common));
	memcpy(&a_depth->tex_params, &tex_common, sizeof(tex_common));

	a_depth->tex_params.type = TEX_TYPE_DEPTH;

	// Foreground: 1 RGB texture per FB
	a_color->tex_params.type = TEX_TYPE_RGB_16;
	stage_draw_fbpair_create(stagedraw.fb_pairs + FBPAIR_FG, 1, a, &rp_fg, "Stage FG");

	// Foreground auxiliary: 1 RGBA texture per FB
	a_color->tex_params.type = TEX_TYPE_RGBA_8;
	stage_draw_fbpair_create(stagedraw.fb_pairs + FBPAIR_FG_AUX, 1, a, &rp_fg_aux, "Stage FG AUX");

	// Background: 1 RGBA texture + depth per FB
	a_color->tex_params.type = TEX_TYPE_RGBA_8;
	stage_draw_fbpair_create(stagedraw.fb_pairs + FBPAIR_BG, 2, a, &rp_bg, "Stage BG");

	// Background auxiliary: 1 RGBA texture per FB
	a_color->tex_params.type = TEX_TYPE_RGBA_8;
	stage_draw_fbpair_create(stagedraw.fb_pairs + FBPAIR_BG_AUX, 1, a, &rp_bg_aux, "Stage BG AUX");

	// CAUTION: should be at least 16-bit, lest the feedback shader do an oopsie!
	a_color->tex_params.type = TEX_TYPE_RGBA_16;
	stagedraw.powersurge_fbpair.front = stage_add_background_framebuffer("Powersurge effect FB 1", 0.125, 0.25, 1, a);
	stagedraw.powersurge_fbpair.back  = stage_add_background_framebuffer("Powersurge effect FB 2", 0.125, 0.25, 1, a);
}

static Framebuffer *add_custom_framebuffer(
	const char *label,
	StageFBPair fbtype,
	float scale_worst,
	float scale_best,
	uint num_attachments,
	FBAttachmentConfig attachments[num_attachments]
) {
	StageFramebufferResizeParams rp = { 0 };
	rp.scaling_base = fbtype;
	rp.scale.worst = scale_worst;
	rp.scale.best = scale_best;

	FramebufferConfig fbconf = { 0 };
	fbconf.attachments = attachments;
	fbconf.num_attachments = num_attachments;
	fbconf.resize_strategy.resize_func = stage_framebuffer_resize_strategy;
	fbconf.resize_strategy.userdata = memdup(&rp, sizeof(rp));
	fbconf.resize_strategy.cleanup_func = stage_framebuffer_resize_strategy_cleanup;
	return fbmgr_group_framebuffer_create(stagedraw.mfb_group, label, &fbconf);
}

Framebuffer *stage_add_foreground_framebuffer(const char *label, float scale_worst, float scale_best, uint num_attachments, FBAttachmentConfig attachments[num_attachments]) {
	return add_custom_framebuffer(label, FBPAIR_FG, scale_worst, scale_best, num_attachments, attachments);
}

Framebuffer *stage_add_background_framebuffer(const char *label, float scale_worst, float scale_best, uint num_attachments, FBAttachmentConfig attachments[num_attachments]) {
	return add_custom_framebuffer(label, FBPAIR_BG, scale_worst, scale_best, num_attachments, attachments);
}

Framebuffer *stage_add_static_framebuffer(const char *label, uint num_attachments, FBAttachmentConfig attachments[num_attachments]) {
	FramebufferConfig fbconf = { 0 };
	fbconf.attachments = attachments;
	fbconf.num_attachments = num_attachments;
	return fbmgr_group_framebuffer_create(stagedraw.mfb_group, label, &fbconf);
}

static void stage_draw_destroy_framebuffers(void) {
	fbmgr_group_destroy(stagedraw.mfb_group);
	stagedraw.mfb_group = NULL;
}

void stage_draw_pre_init(void) {
	stagedraw.mfb_group = fbmgr_group_create();

	preload_resources(RES_POSTPROCESS, RESF_OPTIONAL,
		"viewport",
	NULL);

	preload_resources(RES_SPRITE, RESF_PERMANENT,
		"hud/heart",
		"hud/star",
		"star",
	NULL);

	preload_resources(RES_TEXTURE, RESF_PERMANENT,
		"powersurge_flow",
		"titletransition",
		"hud",
	NULL);

	preload_resources(RES_MODEL, RESF_PERMANENT,
		"hud",
	NULL);

	preload_resources(RES_SHADER_PROGRAM, RESF_PERMANENT,
		"copy_depth",
		"ingame_menu",
		"powersurge_effect",
		"powersurge_feedback",
		"sprite_circleclipped_indicator",
		"text_hud",
		"text_stagetext",

		// TODO: Maybe don't preload this if FXAA is disabled.
		// But then we also have to handle the edge case of it being enabled later mid-game.
		"fxaa",

		#ifdef DEBUG
		"sprite_filled_circle",
		#endif
	NULL);

	preload_resources(RES_FONT, RESF_PERMANENT,
		"mono",
		"small",
		"monosmall",
	NULL);

	stagedraw.framerate_graphs = env_get("TAISEI_FRAMERATE_GRAPHS", GRAPHS_DEFAULT);
	stagedraw.objpool_stats = env_get("TAISEI_OBJPOOL_STATS", OBJPOOLSTATS_DEFAULT);

	if(stagedraw.framerate_graphs) {
		preload_resources(RES_SHADER_PROGRAM, RESF_PERMANENT,
			"graph",
		NULL);
	}

	if(stagedraw.objpool_stats) {
		preload_resources(RES_FONT, RESF_PERMANENT,
			"monotiny",
		NULL);
	}
}

void stage_draw_init(void) {
	stagedraw.viewport_pp = get_resource_data(RES_POSTPROCESS, "viewport", RESF_OPTIONAL);
	stagedraw.hud_text.shader = res_shader("text_hud");
	stagedraw.hud_text.font = res_font("standard");
	stagedraw.shaders.fxaa = res_shader("fxaa");
	stagedraw.shaders.copy_depth = res_shader("copy_depth");

	r_shader_standard();

	#ifdef DEBUG
	stagedraw.dummy.tex = res_sprite("star")->tex;
	stagedraw.dummy.w = 1;
	stagedraw.dummy.h = 1;
	#endif

	stage_draw_setup_framebuffers();

	stagedraw.clear_screen.alpha = 0;
	stagedraw.clear_screen.target_alpha = 0;

	events_register_handler(&(EventHandler) {
		stage_draw_event, NULL, EPRIO_SYSTEM,
	});

	COEVENT_INIT_ARRAY(stagedraw.events);
}

void stage_draw_shutdown(void) {
	COEVENT_CANCEL_ARRAY(stagedraw.events);
	events_unregister_handler(stage_draw_event);
	stage_draw_destroy_framebuffers();
}

FBPair *stage_get_fbpair(StageFBPair id) {
	assert(id >= 0 && id < NUM_FBPAIRS);
	return stagedraw.fb_pairs + id;
}

FBPair *stage_get_postprocess_fbpair(void) {
	return NOT_NULL(stagedraw.current_postprocess_fbpair);
}

static void stage_draw_collision_areas(void) {
#ifdef DEBUG
	static bool enabled, keystate_saved;
	bool keystate = gamekeypressed(KEY_HITAREAS);

	if(keystate ^ keystate_saved) {
		if(keystate) {
			enabled = !enabled;
		}

		keystate_saved = keystate;
	}

	if(!enabled) {
		return;
	}

	r_shader("sprite_filled_circle");
	r_uniform_vec4("color_inner", 0, 0, 0, 1);
	r_uniform_vec4("color_outer", 1, 1, 1, 0.1);

	for(Projectile *p = global.projs.first; p; p = p->next) {
		cmplx gsize = projectile_graze_size(p);

		if(creal(gsize)) {
			r_draw_sprite(&(SpriteParams) {
				.color = RGB(0, 0.5, 0.5),
				.sprite_ptr = &stagedraw.dummy,
				.pos = { creal(p->pos), cimag(p->pos) },
				.rotation.angle = p->angle + M_PI/2,
				.scale = { .x = creal(gsize), .y = cimag(gsize) },
				.blend = BLEND_SUB,
			});
		}
	}

	r_flush_sprites();
	r_uniform_vec4("color_inner", 0.0, 1.0, 0.0, 0.75);
	r_uniform_vec4("color_outer", 0.0, 0.5, 0.5, 0.75);

	for(Projectile *p = global.projs.first; p; p = p->next) {
		r_draw_sprite(&(SpriteParams) {
			.sprite_ptr = &stagedraw.dummy,
			.pos = { creal(p->pos), cimag(p->pos) },
			.rotation.angle = p->angle + M_PI/2,
			.scale = { .x = creal(p->collision_size), .y = cimag(p->collision_size) },
			.blend = BLEND_ALPHA,
		});
	}

	for(Enemy *e = global.enemies.first; e; e = e->next) {
		float hurt_radius = enemy_get_hurt_radius(e);

		if(hurt_radius > 0) {
			r_draw_sprite(&(SpriteParams) {
				.sprite_ptr = &stagedraw.dummy,
				.pos = { creal(e->pos), cimag(e->pos) },
				.scale = { .x = hurt_radius * 2, .y = hurt_radius * 2 },
				.blend = BLEND_ALPHA,
			});
		}
	}

	if(global.boss && boss_is_player_collision_active(global.boss)) {
		r_draw_sprite(&(SpriteParams) {
			.sprite_ptr = &stagedraw.dummy,
			.pos = { creal(global.boss->pos), cimag(global.boss->pos) },
			.scale = { .x = BOSS_HURT_RADIUS * 2, .y = BOSS_HURT_RADIUS * 2 },
			.blend = BLEND_ALPHA,
		});
	}

	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = &stagedraw.dummy,
		.pos = { creal(global.plr.pos), cimag(global.plr.pos) },
		.scale.both = 2, // NOTE: actual player is a singular point
	});

	// TODO: perhaps handle lasers somehow

	r_flush_sprites();
#endif
}

static void apply_shader_rules(ShaderRule *shaderrules, FBPair *fbos) {
	if(!shaderrules) {
		return;
	}

	for(ShaderRule *rule = shaderrules; *rule; ++rule) {
		r_framebuffer(fbos->back);

		if((*rule)(fbos->front)) {
			fbpair_swap(fbos);
		}
	}

	return;
}

static void draw_wall_of_text(float f, const char *txt) {
	Sprite spr;
	BBox bbox;

	char buf[strlen(txt) + 4];
	memcpy(buf, txt, sizeof(buf) - 4);
	memcpy(buf + sizeof(buf) - 4, " ~ ", 4);

	text_render(buf, res_font("standard"), &spr, &bbox);

	// FIXME: The shader currently assumes that the sprite takes up the entire texture.
	// If it could handle any arbitrary sprite, then text_render wouldn't have to resize
	// the texture per every new string of text.

	float w = VIEWPORT_W;
	float h = VIEWPORT_H;

	r_mat_mv_push();
	r_mat_mv_translate(w/2, h/2, 0);
	r_mat_mv_scale(w, h, 1.0);

	uint tw, th;
	r_texture_get_size(spr.tex, 0, &tw, &th);

	r_shader("spellcard_walloftext");
	r_uniform_float("w", spr.tex_area.w);
	r_uniform_float("h", spr.tex_area.h);
	r_uniform_float("ratio", h/w);
	r_uniform_vec2("origin", creal(global.boss->pos)/h, cimag(global.boss->pos)/w); // what the fuck?
	r_uniform_float("t", f);
	r_uniform_sampler("tex", spr.tex);
	r_draw_quad();
	r_shader_standard();

	r_mat_mv_pop();
}

static void draw_spellbg(int t) {
	Boss *b = global.boss;

	r_state_push();
	b->current->draw_rule(b, t);
	r_state_pop();

	if(b->current->type == AT_ExtraSpell) {
		draw_extraspell_bg(b, t);
	}

	float scale = 1;

	if(t < 0) {
		scale = 1.0 - t/attacktype_start_delay(b->current->type);
	}

	r_draw_sprite(&(SpriteParams) {
		.sprite = "boss_spellcircle0",
		.shader = "sprite_default",
		.pos = { creal(b->pos), cimag(b->pos) },
		.rotation.angle = global.frames * 7.0 * DEG2RAD,
		.rotation.vector = { 0, 0, -1 },
		.scale.both = scale,
	});
}

static void draw_spellbg_overlay(int t) {
	Boss *b = global.boss;

	float delay = attacktype_start_delay(b->current->type);
	float f = (ATTACK_START_DELAY - t) / (delay + ATTACK_START_DELAY);

	if(f > 0) {
		draw_wall_of_text(f, b->current->name);
	}
}

static inline bool should_draw_stage_bg(void) {
	if(!global.boss || !global.boss->current)
		return true;

	return (
		!global.boss->current->draw_rule
		|| !attack_is_active(global.boss->current)
		|| global.frames - global.boss->current->starttime < SPELL_INTRO_DURATION
	);
}

static bool fxaa_rule(Framebuffer *fb) {
	r_state_push();
	r_enable(RCAP_DEPTH_TEST);
	r_depth_func(DEPTH_ALWAYS);
	r_blend(BLEND_NONE);
	r_shader_ptr(stagedraw.shaders.fxaa);
	r_uniform_sampler("depth", r_framebuffer_get_attachment(fb, FRAMEBUFFER_ATTACH_DEPTH));
	draw_framebuffer_tex(fb, VIEWPORT_W, VIEWPORT_H);
	r_state_pop();

	return true;
}

static bool copydepth_rule(Framebuffer *fb) {
	r_state_push();
	r_enable(RCAP_DEPTH_TEST);
	r_depth_func(DEPTH_ALWAYS);
	r_blend(BLEND_NONE);
	r_shader_ptr(stagedraw.shaders.copy_depth);

	Framebuffer *target_fb = r_framebuffer_current();
	FramebufferAttachment prev_outputs[FRAMEBUFFER_MAX_OUTPUTS];
	FramebufferAttachment outputs[FRAMEBUFFER_MAX_OUTPUTS];
	for(int i = 0; i < FRAMEBUFFER_MAX_OUTPUTS; ++i) {
		outputs[i] = FRAMEBUFFER_ATTACH_NONE;
	}

	r_framebuffer_get_output_attachments(target_fb, prev_outputs);
	r_framebuffer_set_output_attachments(target_fb, outputs);
	draw_framebuffer_attachment(fb, VIEWPORT_W, VIEWPORT_H, FRAMEBUFFER_ATTACH_DEPTH);
	r_framebuffer_set_output_attachments(target_fb, prev_outputs);
	r_state_pop();

	return false;
}

static bool powersurge_draw_predicate(EntityInterface *ent) {
	uint layer = ent->draw_layer & ~LAYER_LOW_MASK;

	if(player_is_powersurge_active(&global.plr)) {
		switch(layer) {
			case LAYER_PLAYER_SLAVE:
			case LAYER_PLAYER_SHOT:
			case LAYER_PLAYER_SHOT_HIGH:
			case LAYER_PLAYER_FOCUS:
			case LAYER_PLAYER:
				return true;
		}

		if(ent->type == ENT_TYPE_ID(Projectile)) {
			Projectile *p = ENT_CAST(ent, Projectile);
			return p->flags & PFLAG_PLRSPECIALPARTICLE;
		}
	}

	if(ent->type == ENT_TYPE_ID(Item)) {
		Item *i = ENT_CAST(ent, Item);
		return i->type == ITEM_VOLTAGE;
	}

	if(ent->type == ENT_TYPE_ID(YumemiSlave)) {
		return true;
	}

	return false;
}

static bool draw_powersurge_effect(Framebuffer *target_fb, BlendMode blend) {
	r_state_push();
	r_blend(BLEND_NONE);
	r_disable(RCAP_DEPTH_TEST);

	// if(player_is_powersurge_active(&global.plr)) {
		r_state_push();
		r_framebuffer(stagedraw.powersurge_fbpair.front);
		r_blend(BLEND_PREMUL_ALPHA);
		r_shader("sprite_default");
		ent_draw(powersurge_draw_predicate);
		r_state_pop();
	// }

	// TODO: Add heuristic to not run the effect if the buffer can be reasonably assumed to be empty.

	r_shader("powersurge_feedback");
	r_uniform_vec2("blur_resolution", 0.5*VIEWPORT_W, 0.5*VIEWPORT_H);

	r_framebuffer(stagedraw.powersurge_fbpair.back);
	r_uniform_vec2("blur_direction", 1, 0);
	r_uniform_vec4("fade", 1, 1, 1, 1);
	draw_framebuffer_tex(stagedraw.powersurge_fbpair.front, VIEWPORT_W, VIEWPORT_H);
	fbpair_swap(&stagedraw.powersurge_fbpair);

	r_framebuffer(stagedraw.powersurge_fbpair.back);
	r_uniform_vec2("blur_direction", 0, 1);
	r_uniform_vec4("fade", 0.9, 0.9, 0.9, 0.9);
	draw_framebuffer_tex(stagedraw.powersurge_fbpair.front, VIEWPORT_W, VIEWPORT_H);

	r_framebuffer(target_fb);
	r_shader("powersurge_effect");
	r_uniform_sampler("shotlayer", r_framebuffer_get_attachment(stagedraw.powersurge_fbpair.back, FRAMEBUFFER_ATTACH_COLOR0));
	r_uniform_sampler("flowlayer", "powersurge_flow");
	r_uniform_float("time", global.frames/60.0);
	r_blend(blend);
	r_cull(CULL_BACK);
	r_mat_mv_push();
	r_mat_mv_scale(VIEWPORT_W, VIEWPORT_H, 1);
	r_mat_mv_translate(0.5, 0.5, 0);
	r_draw_quad();
	r_mat_mv_pop();
	fbpair_swap(&stagedraw.powersurge_fbpair);

	r_state_pop();

	return true;
}

static bool boss_distortion_rule(Framebuffer *fb) {
	if(global.boss == NULL) {
		return false;
	}

	r_state_push();
	r_blend(BLEND_NONE);
	r_disable(RCAP_DEPTH_TEST);

	cmplx fpos = global.boss->pos;;
	cmplx pos = fpos + 15 * cdir(global.frames/12.0);

	r_shader("boss_zoom");
	r_uniform_vec2("blur_orig", creal(pos)  / VIEWPORT_W,  1-cimag(pos)  / VIEWPORT_H);
	r_uniform_vec2("fix_orig",  creal(fpos) / VIEWPORT_W,  1-cimag(fpos) / VIEWPORT_H);
	r_uniform_float("blur_rad", 1.5*(0.2+0.025*sin(global.frames/15.0)));
	r_uniform_float("rad", 0.24);
	r_uniform_float("ratio", (float)VIEWPORT_H/VIEWPORT_W);
	r_uniform_vec4_rgba("color", &global.boss->zoomcolor);
	draw_framebuffer_tex(fb, VIEWPORT_W, VIEWPORT_H);

	r_state_pop();

	return true;
}

static void finish_3d_scene(FBPair *fbpair) {
	// Here we synchronize the depth buffers of both framebuffers in the pair.
	// The FXAA shader has this built-in, so we don't need to do the copy_depth
	// pass in that case.

	// The reason for this is that, normally, depth testing is disabled during
	// the stage-specific post-processing ping-pong. This applies to depth writes
	// as well. Therefore, only one of the framebuffers will get a depth write in
	// a given frame. In the worst case, this will be the same framebuffer every
	// time, leaving the other one's depth buffer undefined. In the best case,
	// they will alternate. If a shader down the post-processing pipeline happens
	// to sample the "unlucky" buffer, it'll probably either completely destroy
	// the background rendering, or the sample will lag 1 frame behind.

	// This flew past the radar for a long time. Now that I actually had to waste
	// my evening debugging this peculiarity, I figured I might as well document
	// it here. It is possible to solve this problem without an additional pass,
	// but that would require a bit of refactoring. This is the simplest solution
	// as far as I can tell.

	apply_shader_rules((ShaderRule[]) {
		/*
		config_get_int(CONFIG_FXAA)
			? fxaa_rule
			: copydepth_rule,
		NULL
		*/
		copydepth_rule, NULL
	}, fbpair);
}

static void draw_full_spellbg(int t, FBPair *fbos) {
	BlendMode blend_old = r_blend_current();
	r_framebuffer(fbos->back);
	r_blend(BLEND_PREMUL_ALPHA);
	draw_spellbg(t);
	fbpair_swap(fbos);
	r_blend(BLEND_NONE);
	apply_shader_rules((ShaderRule[]) { boss_distortion_rule, NULL }, fbos);
	r_blend(BLEND_PREMUL_ALPHA);
	draw_spellbg_overlay(t);
	r_blend(blend_old);
}

static void apply_bg_shaders(ShaderRule *shaderrules, FBPair *fbos) {
	Boss *b = global.boss;

	r_state_push();
	r_blend(BLEND_NONE);

	if(should_draw_stage_bg()) {
		finish_3d_scene(fbos);
		apply_shader_rules(shaderrules, fbos);

		// anti-aliasing
		if(config_get_int(CONFIG_FXAA)) {
			apply_shader_rules((ShaderRule[]) { fxaa_rule, NULL } , fbos);
		}
	}

	if(b && b->current && b->current->draw_rule && b->current->starttime > 0) {
		int t = global.frames - b->current->starttime;
		int delay = attacktype_start_delay(b->current->type);

		bool trans_intro = t + delay < SPELL_INTRO_DURATION;
		bool trans_outro = attack_has_finished(b->current);

		if(!trans_intro && !trans_outro) {
			draw_full_spellbg(t, fbos);
		} else {
			FBPair *aux = stage_get_fbpair(FBPAIR_BG_AUX);
			draw_full_spellbg(t, aux);

			apply_shader_rules((ShaderRule[]) { boss_distortion_rule, NULL }, fbos);
			fbpair_swap(fbos);
			r_framebuffer(fbos->back);

			cmplx pos = b->pos;
			float ratio = (float)VIEWPORT_H/VIEWPORT_W;

			if(trans_intro) {
				r_shader("spellcard_intro");
				r_uniform_float("ratio", ratio);
				r_uniform_vec2("origin", creal(pos) / VIEWPORT_W, 1 - cimag(pos) / VIEWPORT_H);
				r_uniform_float("t", SPELL_INTRO_TIME_FACTOR * (t + delay) / (float)SPELL_INTRO_DURATION);
			} else {
				int tn = global.frames - b->current->endtime;
				delay = b->current->endtime - b->current->endtime_undelayed;

				r_shader("spellcard_outro");
				r_uniform_float("ratio", ratio);
				r_uniform_vec2("origin", creal(pos) / VIEWPORT_W, 1 - cimag(pos) / VIEWPORT_H);
				r_uniform_float("t", fmax(0, tn / (float)delay + 1));
			}

			r_blend(BLEND_PREMUL_ALPHA);
			draw_framebuffer_tex(aux->front, VIEWPORT_W, VIEWPORT_H);
			r_blend(BLEND_NONE);
			fbpair_swap(fbos);
		}
	} else {
		apply_shader_rules((ShaderRule[]) { boss_distortion_rule, NULL }, fbos);
	}

	r_state_pop();
}

static void stage_render_bg(StageInfo *stage) {
	FBPair *background = stage_get_fbpair(FBPAIR_BG);

	r_framebuffer(background->back);
	r_clear(CLEAR_ALL, RGBA(0, 0, 0, 1), 1);

	if(should_draw_stage_bg()) {
		r_mat_mv_push();
		r_enable(RCAP_DEPTH_TEST);
		stage->procs->draw();
		r_mat_mv_pop();
		fbpair_swap(background);
	}

	set_ortho(VIEWPORT_W, VIEWPORT_H);
	r_disable(RCAP_DEPTH_TEST);

	apply_bg_shaders(stage->procs->shader_rules, background);

	int pp = config_get_int(CONFIG_POSTPROCESS);

	if(pp > 1) {
		draw_powersurge_effect(background->front, BLEND_PREMUL_ALPHA);
	} else if(pp > 0) {
		Framebuffer *staging = stage_get_fbpair(FBPAIR_BG_AUX)->back;

		r_state_push();
		r_framebuffer_clear(staging, CLEAR_COLOR, RGBA(0, 0, 0, 0), 1);
		draw_powersurge_effect(staging, BLEND_NONE);
		r_shader_standard();
		r_framebuffer(background->front);
		r_blend(BLEND_PREMUL_ALPHA);
		draw_framebuffer_tex(staging, VIEWPORT_W, VIEWPORT_H);
		r_state_pop();
	}
}

bool stage_should_draw_particle(Projectile *p) {
	return (p->flags & PFLAG_REQUIREDPARTICLE) || config_get_int(CONFIG_PARTICLES);
}

static bool stage_draw_predicate(EntityInterface *ent) {
	if(ent->type == ENT_TYPE_ID(Projectile)) {
		Projectile *p = ENT_CAST(ent, Projectile);

		if(p->type == PROJ_PARTICLE) {
			return stage_should_draw_particle(p);
		}
	}

	return true;
}

static void stage_draw_objects(void) {
	r_shader("sprite_default");

	if(global.boss) {
		draw_boss_background(global.boss);
	}

	ent_draw(
		config_get_int(CONFIG_PARTICLES)
			? NULL
			: stage_draw_predicate
	);

	if(global.boss) {
		draw_boss_fake_overlay(global.boss);
	}

	stage_draw_collision_areas();
	r_shader_standard();
}

void stage_draw_overlay(void) {
	r_state_push();
	r_shader("sprite_default");
	r_blend(BLEND_PREMUL_ALPHA);

	if(global.boss) {
		draw_boss_overlay(global.boss);
	}

	if(stagedraw.clear_screen.alpha > 0) {
		fade_out(stagedraw.clear_screen.alpha * 0.5);
	}

	r_shader_standard();
	stagetext_draw();
	player_draw_overlay(&global.plr);
	r_state_pop();
}

static void postprocess_prepare(Framebuffer *fb, ShaderProgram *s, void *arg) {
	r_uniform_int("frames", global.frames);
	r_uniform_vec2("viewport", VIEWPORT_W, VIEWPORT_H);
	r_uniform_vec2("player", creal(global.plr.pos), VIEWPORT_H - cimag(global.plr.pos));
}

static inline void begin_viewport_shake(void) {
	float s = stage_get_view_shake_strength();

	if(s > 0) {
// 		s = log1pf(s);
		s = sqrtf(s);

		r_mat_mv_push();
		r_mat_mv_translate(
			s * sin(global.frames),
			s * sin(global.frames * 1.1 + 3),
			0
		);
		r_mat_mv_scale(
			1 + 2 * s / VIEWPORT_W,
			1 + 2 * s / VIEWPORT_H,
			1
		);
		r_mat_mv_translate(
			-s,
			-s,
			0
		);
	}
}

static inline void end_viewport_shake(void) {
	if(stage_get_view_shake_strength() > 0) {
		r_mat_mv_pop();
	}
}

/*
 * Small helpers for entities draw code that might want to suppress viewport shake temporarily.
 * This is mostly useful when multiple framebuffers are involved.
 */
static int shake_suppressed = 0;

void stage_draw_begin_noshake(void) {
	assert(!shake_suppressed);
	shake_suppressed = 1;

	if(stage_get_view_shake_strength() > 0) {
		shake_suppressed = 2;
		r_mat_mv_push_identity();
	}
}

void stage_draw_end_noshake(void) {
	assert(shake_suppressed);

	if(stage_get_view_shake_strength() > 0) {
		// make sure shake_view doesn't change in-between the begin/end calls;
		// that would've been *really* nasty to debug.
		assert(shake_suppressed == 2);
		r_mat_mv_pop();
	}

	shake_suppressed = 0;
}

void stage_draw_viewport(void) {
	FloatRect dest_vp;
	r_framebuffer_viewport_current(r_framebuffer_current(), &dest_vp);
	r_uniform_sampler("tex", r_framebuffer_get_attachment(stagedraw.fb_pairs[FBPAIR_FG].front, FRAMEBUFFER_ATTACH_COLOR0));

	// CAUTION: Very intricate pixel perfect scaling that will ruin your day.
	float facw = dest_vp.w / SCREEN_W;
	float fach = dest_vp.h / SCREEN_H;
	// fach is equal to facw up to roundoff error.
	float scale = fach;

	r_mat_mv_push();
	r_mat_mv_scale(1/facw, 1/fach, 1);
	r_mat_mv_translate(roundf(facw * VIEWPORT_X), roundf(fach * VIEWPORT_Y), 0);
	r_mat_mv_scale(roundf(scale * VIEWPORT_W), roundf(scale * VIEWPORT_H), 1);
	r_mat_mv_translate(0.5, 0.5, 0);
	r_draw_quad();
	r_mat_mv_pop();
}

void stage_draw_scene(StageInfo *stage) {
#ifdef DEBUG
	bool key_nobg = gamekeypressed(KEY_NOBACKGROUND);
#else
	bool key_nobg = false;
#endif

	FBPair *background = stage_get_fbpair(FBPAIR_BG);
	FBPair *foreground = stage_get_fbpair(FBPAIR_FG);

	bool draw_bg = !config_get_int(CONFIG_NO_STAGEBG) && !key_nobg;

	if(draw_bg) {
		stage_render_bg(stage);
	}

	// prepare for 2D rendering into the game viewport framebuffer
	r_framebuffer(foreground->back);
	set_ortho(VIEWPORT_W, VIEWPORT_H);
	r_disable(RCAP_DEPTH_TEST);
	r_blend(BLEND_PREMUL_ALPHA);
	r_cull(CULL_BACK);
	r_shader_standard();

	begin_viewport_shake();

	if(draw_bg) {
		// blit the background
		r_state_push();
		r_blend(BLEND_NONE);
		draw_framebuffer_tex(background->front, VIEWPORT_W, VIEWPORT_H);
		r_state_pop();

		coevent_signal(&stagedraw.events.background_drawn);
	} else if(!key_nobg) {
		r_clear(CLEAR_COLOR, RGBA(0, 0, 0, 1), 1);
	}

	// draw the 2D objects
	stage_draw_objects();

	end_viewport_shake();

	// prepare to apply postprocessing
	fbpair_swap(foreground);
	r_blend(BLEND_NONE);

	stagedraw.current_postprocess_fbpair = foreground;

	coevent_signal(&stagedraw.events.postprocess_before_overlay);

	// draw overlay: in-viewport text and HUD elements, etc.
	// this stuff is not affected by the screen shake effect
	stage_draw_overlay();

	coevent_signal(&stagedraw.events.postprocess_after_overlay);

	// stage postprocessing
	apply_shader_rules(global.stage->procs->postprocess_rules, foreground);

	// custom postprocessing
	postprocess(
		stagedraw.viewport_pp,
		foreground,
		postprocess_prepare,
		draw_framebuffer_tex,
		VIEWPORT_W,
		VIEWPORT_H,
		NULL
	);

	stagedraw.current_postprocess_fbpair = NULL;

	// prepare for 2D rendering into the main framebuffer (actual screen)
	r_framebuffer(video_get_screen_framebuffer());
	set_ortho(SCREEN_W, SCREEN_H);

	// draw viewport contents
	stage_draw_viewport();

	// draw HUD
	stage_draw_hud();

	// draw dialog
	dialog_draw(global.dialog);

	// draw "bottom text" (FPS, replay info, etc.)
	stage_draw_bottom_text();
}

#define HUD_X_PADDING 16
#define HUD_X_OFFSET (VIEWPORT_W + VIEWPORT_X)
#define HUD_WIDTH (SCREEN_W - HUD_X_OFFSET)
#define HUD_EFFECTIVE_WIDTH (HUD_WIDTH - HUD_X_PADDING * 2)
#define HUD_X_SECONDARY_OFS_ICON 18
#define HUD_X_SECONDARY_OFS_LABEL (HUD_X_SECONDARY_OFS_ICON + 12)
#define HUD_X_SECONDARY_OFS_VALUE (HUD_X_SECONDARY_OFS_LABEL + 60)

struct glyphcb_state {
	Color *color1, *color2;
};

static int draw_numeric_callback(Font *font, charcode_t charcode, SpriteInstanceAttribs *spr_attribs, void *userdata) {
	struct glyphcb_state *st = userdata;

	if(charcode != '0' && charcode != ',') {
		st->color1 = st->color2;
	}

	spr_attribs->rgba = *st->color1;
	return 0;
}

static inline void stage_draw_hud_power_value(float xpos, float ypos) {
	Font *fnt_int = res_font("standard");
	Font *fnt_fract = res_font("small");

	int pw = global.plr.power + global.plr.power_overflow;

	Color *c_whole, c_whole_buf, *c_fract, c_fract_buf;
	Color *c_op_mod = RGBA(1, 0.2 + 0.3 * psin(global.frames / 10.0), 0.2, 1.0);

	if(pw <= PLR_MAX_POWER) {
		c_whole = &stagedraw.hud_text.color.active;
		c_fract = &stagedraw.hud_text.color.inactive;
	} else if(pw - PLR_MAX_POWER < 100) {
		c_whole = &stagedraw.hud_text.color.active;
		c_fract = color_mul(color_copy(&c_fract_buf, &stagedraw.hud_text.color.inactive), c_op_mod);
	} else {
		c_whole = color_mul(color_copy(&c_whole_buf, &stagedraw.hud_text.color.active), c_op_mod);
		c_fract = color_mul(color_copy(&c_fract_buf, &stagedraw.hud_text.color.inactive), c_op_mod);
	}

	xpos = draw_fraction(
		pw / 100.0,
		ALIGN_LEFT,
		xpos,
		ypos,
		fnt_int,
		fnt_fract,
		c_whole,
		c_fract,
		false
	);

	xpos += text_draw(" / ", &(TextParams) {
		.pos = { xpos, ypos },
		.color = &stagedraw.hud_text.color.active,
		.align = ALIGN_LEFT,
		.font_ptr = fnt_int,
	});

	draw_fraction(
		PLR_MAX_POWER / 100.0,
		ALIGN_LEFT,
		xpos,
		ypos,
		fnt_int,
		fnt_fract,
		&stagedraw.hud_text.color.active,
		&stagedraw.hud_text.color.inactive,
		false
	);
}

static void stage_draw_hud_score(Alignment a, float xpos, float ypos, char *buf, size_t bufsize, uint32_t score) {
	format_huge_num(10, score, bufsize, buf);

	Font *fnt = res_font("standard");
	bool kern_saved = font_get_kerning_enabled(fnt);
	font_set_kerning_enabled(fnt, false);

	text_draw(buf, &(TextParams) {
		.pos = { xpos, ypos },
		.font = "standard",
		.align = ALIGN_RIGHT,
		.glyph_callback = {
			draw_numeric_callback,
			&(struct glyphcb_state) { &stagedraw.hud_text.color.inactive, &stagedraw.hud_text.color.active },
		}
	});

	font_set_kerning_enabled(fnt, kern_saved);
}

static void stage_draw_hud_scores(float ypos_hiscore, float ypos_score, char *buf, size_t bufsize) {
	stage_draw_hud_score(ALIGN_RIGHT, HUD_EFFECTIVE_WIDTH, ypos_hiscore, buf, bufsize, progress.hiscore);
	stage_draw_hud_score(ALIGN_RIGHT, HUD_EFFECTIVE_WIDTH, ypos_score,   buf, bufsize, global.plr.points);
}

static void stage_draw_hud_objpool_stats(float x, float y, float width) {
	ObjectPool **last = &stage_object_pools.first + (sizeof(StageObjectPools)/sizeof(ObjectPool*) - 1);
	Font *font = res_font("monotiny");

	ShaderProgram *sh_prev = r_shader_current();
	r_shader("text_default");
	for(ObjectPool **pool = &stage_object_pools.first; pool <= last; ++pool) {
		ObjectPoolStats stats;
		char buf[32];
		objpool_get_stats(*pool, &stats);

		snprintf(buf, sizeof(buf), "%zu | %5zu", stats.usage, stats.peak_usage);
		// draw_text(ALIGN_LEFT  | AL_Flag_NoAdjust, (int)x,           (int)y, stats.tag, font);
		// draw_text(ALIGN_RIGHT | AL_Flag_NoAdjust, (int)(x + width), (int)y, buf,       font);
		// y += stringheight(buf, font) * 1.1;

		text_draw(stats.tag, &(TextParams) {
			.pos = { x, y },
			.font_ptr = font,
			.align = ALIGN_LEFT,
		});

		text_draw(buf, &(TextParams) {
			.pos = { x + width, y },
			.font_ptr = font,
			.align = ALIGN_RIGHT,
		});

		y += font_get_lineskip(font);
	}
	r_shader_ptr(sh_prev);
}

struct labels_s {
	struct {
		float next_life;
		float next_bomb;
	} x;

	struct {
		float hiscore;
		float score;
		float lives;
		float bombs;
		float power;
		float value;
		float voltage;
		float graze;
	} y;

	struct {
		float lives_display;
		float lives_text;
		float bombs_display;
		float bombs_text;
	} y_ofs;

	Color lb_baseclr;
};

static void draw_graph(float x, float y, float w, float h) {
	r_mat_mv_push();
	r_mat_mv_translate(x + w/2, y + h/2, 0);
	r_mat_mv_scale(w, h, 1);
	r_draw_quad();
	r_mat_mv_pop();
}

static void draw_label(const char *label_str, double y_ofs, struct labels_s* labels, Color *clr) {
	text_draw(label_str, &(TextParams) {
		.font_ptr = stagedraw.hud_text.font,
		.shader_ptr = stagedraw.hud_text.shader,
		.pos = { 0, y_ofs },
		.color = clr,
	});
}

static void stage_draw_hud_text(struct labels_s* labels) {
	char buf[64];
	Font *font;
	bool kern_saved;

	r_shader_ptr(stagedraw.hud_text.shader);

	Color *lb_label_clr = color_mul(COLOR_COPY(&labels->lb_baseclr), &stagedraw.hud_text.color.label);

	// Labels
	draw_label("Hi-Score:",    labels->y.hiscore, labels, &stagedraw.hud_text.color.label);
	draw_label("Score:",       labels->y.score,   labels, &stagedraw.hud_text.color.label);
	draw_label("Lives:",       labels->y.lives,   labels, lb_label_clr);
	draw_label("Spell Cards:", labels->y.bombs,   labels, lb_label_clr);

	r_mat_mv_push();
	r_mat_mv_translate(HUD_X_SECONDARY_OFS_LABEL, 0, 0);
	draw_label("Power:",       labels->y.power,   labels, &stagedraw.hud_text.color.label_power);
	draw_label("Value:",       labels->y.value,   labels, &stagedraw.hud_text.color.label_value);
	draw_label("Volts:",       labels->y.voltage, labels, &stagedraw.hud_text.color.label_voltage);
	draw_label("Graze:",       labels->y.graze,   labels, &stagedraw.hud_text.color.label_graze);
	r_mat_mv_pop();

	if(stagedraw.objpool_stats) {
		stage_draw_hud_objpool_stats(0, 390, HUD_EFFECTIVE_WIDTH);
	}

	// Score/Hi-Score values
	stage_draw_hud_scores(labels->y.hiscore, labels->y.score, buf, sizeof(buf));

	// Lives and Bombs (N/A)
	if(global.stage->type == STAGE_SPELL) {
		r_color(color_mul_scalar(COLOR_COPY(&labels->lb_baseclr), 0.7));
		text_draw("N/A", &(TextParams) { .pos = { HUD_EFFECTIVE_WIDTH, labels->y.lives }, .font_ptr = stagedraw.hud_text.font, .align = ALIGN_RIGHT });
		text_draw("N/A", &(TextParams) { .pos = { HUD_EFFECTIVE_WIDTH, labels->y.bombs }, .font_ptr = stagedraw.hud_text.font, .align = ALIGN_RIGHT });
		r_color4(1, 1, 1, 1.0);
	}

	const float res_text_padding = 4;

	// Score left to next extra life
	if(labels->x.next_life > 0) {
		Color *next_clr = color_mul(RGBA(0.5, 0.3, 0.4, 0.5), &labels->lb_baseclr);
		format_huge_num(0, global.plr.extralife_threshold - global.plr.points, sizeof(buf), buf);
		font = res_font("small");

		text_draw("Next:", &(TextParams) {
			.pos = { labels->x.next_life + res_text_padding, labels->y.lives + labels->y_ofs.lives_text },
			.font_ptr = font,
			.align = ALIGN_LEFT,
			.color = next_clr,
		});

		kern_saved = font_get_kerning_enabled(font);
		font_set_kerning_enabled(font, false);

		text_draw(buf, &(TextParams) {
			.pos = { HUD_EFFECTIVE_WIDTH - res_text_padding, labels->y.lives + labels->y_ofs.lives_text },
			.font_ptr = font,
			.align = ALIGN_RIGHT,
			.color = next_clr,
		});

		font_set_kerning_enabled(font, kern_saved);
	}

	// Bomb fragments (numeric)
	if(labels->x.next_bomb > 0) {
		Color *next_clr = color_mul(RGBA(0.3, 0.5, 0.3, 0.5), &labels->lb_baseclr);
		snprintf(buf, sizeof(buf), "%d / %d", global.plr.bomb_fragments, PLR_MAX_BOMB_FRAGMENTS);
		font = res_font("small");

		text_draw("Fragments:", &(TextParams) {
			.pos = { labels->x.next_bomb + res_text_padding, labels->y.bombs + labels->y_ofs.bombs_text },
			.font_ptr = font,
			.align = ALIGN_LEFT,
			.color = next_clr,
		});

		kern_saved = font_get_kerning_enabled(font);
		font_set_kerning_enabled(font, false);

		text_draw(buf, &(TextParams) {
			.pos = { HUD_EFFECTIVE_WIDTH - res_text_padding, labels->y.bombs + labels->y_ofs.bombs_text },
			.font_ptr = font,
			.align = ALIGN_RIGHT,
			.color = next_clr,
		});

		font_set_kerning_enabled(font, kern_saved);
	}

	r_mat_mv_push();
	r_mat_mv_translate(HUD_X_SECONDARY_OFS_VALUE, 0, 0);

	// Power value
	stage_draw_hud_power_value(0, labels->y.power);

	font = res_font("standard");
	kern_saved = font_get_kerning_enabled(font);
	font_set_kerning_enabled(font, false);

	// Point Item Value... value
	format_huge_num(6, global.plr.point_item_value, sizeof(buf), buf);
	text_draw(buf, &(TextParams) {
		.pos = { 0, labels->y.value },
		.shader_ptr = stagedraw.hud_text.shader,
		.font_ptr = font,
		.glyph_callback = {
			draw_numeric_callback,
			&(struct glyphcb_state) { &stagedraw.hud_text.color.inactive, &stagedraw.hud_text.color.active },
		}
	});

	// Voltage value
	format_huge_num(4, global.plr.voltage, sizeof(buf), buf);
	float volts_x = 0;

	Color *voltage_tint = global.plr.voltage >= global.voltage_threshold
		? RGB(1.0, 0.9, 0.7) // RGB(0.9, 0.7, 1.0)
		: RGB(1.0, 1.0, 1.0);

	volts_x += text_draw(buf, &(TextParams) {
		.pos = { volts_x, labels->y.voltage },
		.shader_ptr = stagedraw.hud_text.shader,
		.font_ptr = font,
		.glyph_callback = {
			draw_numeric_callback,
			&(struct glyphcb_state) {
				color_mul(COLOR_COPY(&stagedraw.hud_text.color.inactive), voltage_tint),
				color_mul(COLOR_COPY(&stagedraw.hud_text.color.active), voltage_tint),
			},
		}
	});

	volts_x += text_draw("V", &(TextParams) {
		.pos = { volts_x, labels->y.voltage },
		.shader_ptr = stagedraw.hud_text.shader,
		.font_ptr = font,
		.color = &stagedraw.hud_text.color.dark,
	});

	volts_x += text_draw(" / ", &(TextParams) {
		.pos = { volts_x, labels->y.voltage },
		.shader_ptr = stagedraw.hud_text.shader,
		.font_ptr = font,
		.color = &stagedraw.hud_text.color.active,
	});

	format_huge_num(4, global.voltage_threshold, sizeof(buf), buf);

	volts_x += text_draw(buf, &(TextParams) {
		.pos = { volts_x, labels->y.voltage },
		.shader_ptr = stagedraw.hud_text.shader,
		.font_ptr = font,
		.glyph_callback = {
			draw_numeric_callback,
			&(struct glyphcb_state) { &stagedraw.hud_text.color.inactive, &stagedraw.hud_text.color.active },
		}
	});

	volts_x += text_draw("V", &(TextParams) {
		.pos = { volts_x, labels->y.voltage },
		.shader_ptr = stagedraw.hud_text.shader,
		.font_ptr = font,
		.color = &stagedraw.hud_text.color.dark,
	});

	// Graze value
	format_huge_num(6, global.plr.graze, sizeof(buf), buf);
	text_draw(buf, &(TextParams) {
		.pos = { 0, labels->y.graze },
		.shader_ptr = stagedraw.hud_text.shader,
		.font_ptr = font,
		.glyph_callback = {
			draw_numeric_callback,
			&(struct glyphcb_state) { &stagedraw.hud_text.color.inactive, &stagedraw.hud_text.color.active  },
		}
	});

	font_set_kerning_enabled(font, kern_saved);
	r_mat_mv_pop();

	// God Mode indicator
	if(global.plr.iddqd) {
		text_draw("God Mode is enabled!", &(TextParams) {
			.pos = { HUD_EFFECTIVE_WIDTH * 0.5, 450 },
			.font_ptr = font,
			.shader_ptr = stagedraw.hud_text.shader,
			.align = ALIGN_CENTER,
			.color = RGB(1.0, 0.5, 0.2),
		});
	}
}

void stage_draw_bottom_text(void) {
	char buf[64];
	Font *font;

#ifdef DEBUG
	snprintf(buf, sizeof(buf), "%.2f lfps, %.2f rfps, timer: %d, frames: %d (%d:%02d) ",
		global.fps.logic.fps,
		global.fps.render.fps,
		global.timer,
		global.frames,
		global.frames / 3600,
		(global.frames % 3600) / 60
	);
#else
	if(get_effective_frameskip() > 1) {
		snprintf(buf, sizeof(buf), "%.2f lfps, %.2f rfps",
			global.fps.logic.fps,
			global.fps.render.fps
		);
	} else {
		snprintf(buf, sizeof(buf), "%.2f fps",
			global.fps.logic.fps
		);
	}
#endif

	font = res_font("monosmall");

	text_draw(buf, &(TextParams) {
		.align = ALIGN_RIGHT,
		.pos = { SCREEN_W, SCREEN_H - 0.5 * text_height(font, buf, 0) },
		.font_ptr = font,
	});

	if(global.replay.input.replay != NULL) {
		ReplayState *rst = &global.replay.input;

		r_shader_ptr(stagedraw.hud_text.shader);
		// XXX: does it make sense to use the monospace font here?

		snprintf(buf, sizeof(buf), "Replay: %s (%i fps)", rst->replay->playername, rst->play.fps);
		int x = 0, y = SCREEN_H - 0.5 * text_height(font, buf, 0);

		x += text_draw(buf, &(TextParams) {
			.pos = { x, y },
			.font_ptr = font,
			.color = &stagedraw.hud_text.color.inactive,
		});

		if(rst->play.desync_frame >= 0) {
			snprintf(buf, sizeof(buf), " (DESYNCED near frame #%i)", rst->play.desync_frame);

			text_draw(buf, &(TextParams) {
				.pos = { x, y },
				.font_ptr = font,
				.color = RGBA_MUL_ALPHA(1.00, 0.20, 0.20, 0.60),
			});
		}
	}
#ifdef PLR_DPS_STATS
	else if(global.frames) {
		int totaldmg = 0;
		int framespan = sizeof(global.plr.dmglog)/sizeof(*global.plr.dmglog);
		int graphspan = framespan;
		static int max = 0;
		float graph[framespan];

		if(graphspan > 120) {
			// shader limitation
			graphspan = 120;
		}

		// hack to update the graph every frame
		player_register_damage(&global.plr, NULL, &(DamageInfo) { .amount = 0, .type = DMG_PLAYER_SHOT });

		for(int i = 0; i < framespan; ++i) {
			totaldmg += global.plr.dmglog[i];

			if(global.plr.dmglog[i] > max) {
				max = global.plr.dmglog[i];
			}
		}

		for(int i = 0; i < graphspan; ++i) {
			if(max > 0) {
				graph[i] = (float)global.plr.dmglog[i] / max;
			} else {
				graph[i] = 0;
			}
		}

		snprintf(buf, sizeof(buf), "%.02f", totaldmg / (framespan / (double)FPS));
		double text_h = text_height(font, buf, 0);
		double x = 0, y = SCREEN_H - 0.5 * text_h;

		x += text_draw("Avg DPS: ", &(TextParams) {
			.pos = { x, y },
			.font_ptr = font,
			.color = &stagedraw.hud_text.color.inactive,
		});

		text_draw(buf, &(TextParams) {
			.pos = { x, y },
			.font_ptr = font,
			.color = &stagedraw.hud_text.color.active,
		});

		r_shader("graph");
		r_uniform_vec3("color_low",  1.0, 0.0, 0.0);
		r_uniform_vec3("color_mid",  1.0, 1.0, 0.0);
		r_uniform_vec3("color_high", 0.0, 1.0, 0.0);
		r_uniform_float_array("points[0]", 0, graphspan, graph);
		draw_graph(142, SCREEN_H - text_h, graphspan, text_h);
	}
#endif

	r_shader_standard();
}

static void fill_graph(int num_samples, float *samples, FPSCounter *fps) {
	for(int i = 0; i < num_samples; ++i) {
		samples[i] = fps->frametimes[i] / (2.0 * (HRTIME_RESOLUTION / (long double)FPS));

		if(samples[i] > 1.0) {
			samples[i] = 1.0;
		}
	}
}

static void stage_draw_framerate_graphs(float x, float y, float w, float h) {
	#define NUM_SAMPLES (sizeof(((FPSCounter){{0}}).frametimes) / sizeof(((FPSCounter){{0}}).frametimes[0]))
	static float samples[NUM_SAMPLES];

	r_state_push();
	r_shader("graph");

	fill_graph(NUM_SAMPLES, samples, &global.fps.logic);
	r_uniform_vec3("color_low",  0.0, 1.0, 1.0);
	r_uniform_vec3("color_mid",  1.0, 1.0, 0.0);
	r_uniform_vec3("color_high", 1.0, 0.0, 0.0);
	r_uniform_float_array("points[0]", 0, NUM_SAMPLES, samples);
	draw_graph(x, y, w, h);

	// x -= w * 1.1;
	y += h + 1;

	fill_graph(NUM_SAMPLES, samples, &global.fps.busy);
	r_uniform_vec3("color_low",  0.0, 1.0, 0.0);
	r_uniform_vec3("color_mid",  1.0, 0.0, 0.0);
	r_uniform_vec3("color_high", 1.0, 0.0, 0.5);
	r_uniform_float_array("points[0]", 0, NUM_SAMPLES, samples);
	draw_graph(x, y, w, h);

	r_state_pop();
}

void stage_draw_hud(void) {
	// Background
	r_mat_mv_push();
	r_mat_mv_translate(SCREEN_W * 0.5, SCREEN_H * 0.5, 0);
	r_mat_mv_scale(SCREEN_W, SCREEN_W, 1);
	r_shader_standard();
	r_uniform_sampler("tex", "hud");
	r_draw_model("hud");
	r_mat_mv_pop();

	r_blend(BLEND_PREMUL_ALPHA);

	// Set up positions of most HUD elements
	struct labels_s labels = { 0 };

	const float label_spacing = 32;
	float label_ypos = 0;

	label_ypos = 16;
	labels.y.hiscore = label_ypos += label_spacing;
	labels.y.score   = label_ypos += label_spacing;

	label_ypos = 108;
	labels.y.lives   = label_ypos += label_spacing;
	labels.y.bombs   = label_ypos += label_spacing * 1.25;

	label_ypos = 210;
	labels.y.power   = label_ypos += label_spacing;
	labels.y.value   = label_ypos += label_spacing;
	labels.y.voltage = label_ypos += label_spacing;
	labels.y.graze   = label_ypos += label_spacing;

	r_mat_mv_push();
	r_mat_mv_translate(HUD_X_OFFSET + HUD_X_PADDING, 0, 0);

	// Set up Extra Spell indicator opacity early; some other elements depend on it
	float extraspell_alpha = 0;
	float extraspell_fadein = 1;

	if(global.boss && global.boss->current && global.boss->current->type == AT_ExtraSpell) {
		extraspell_fadein  = fmin(1, -fmin(0, global.frames - global.boss->current->starttime) / (float)ATTACK_START_DELAY);

		float fadeout;

		if(attack_has_finished(global.boss->current)) {
			fadeout = (1 - (global.boss->current->endtime - global.frames) / (float)ATTACK_END_DELAY_EXTRA) / 0.74;
		} else {
			fadeout = 0;
		}

		float fade = fmax(extraspell_fadein, fadeout);
		extraspell_alpha = 1 - fade;
	}

	labels.lb_baseclr.r = 1 - extraspell_alpha;
	labels.lb_baseclr.g = 1 - extraspell_alpha;
	labels.lb_baseclr.b = 1 - extraspell_alpha;
	labels.lb_baseclr.a = 1 - extraspell_alpha * 0.5;

	// Lives and Bombs
	if(global.stage->type != STAGE_SPELL) {
		r_mat_mv_push();
		r_mat_mv_translate(0, font_get_descent(res_font("standard")), 0);

		Sprite *spr_life = res_sprite("hud/heart");
		Sprite *spr_bomb = res_sprite("hud/star");

		float spacing = 1;
		float pos_lives = HUD_EFFECTIVE_WIDTH - spr_life->w * (PLR_MAX_LIVES - 0.5) - spacing * (PLR_MAX_LIVES - 1);
		float pos_bombs = HUD_EFFECTIVE_WIDTH - spr_bomb->w * (PLR_MAX_BOMBS - 0.5) - spacing * (PLR_MAX_BOMBS - 1);

		labels.y_ofs.lives_display = 0 /* spr_life->h * -0.25 */;
		labels.y_ofs.bombs_display = 0 /* spr_bomb->h * -0.25 */;

		labels.y_ofs.lives_text = labels.y_ofs.lives_display + spr_life->h;
		labels.y_ofs.bombs_text = labels.y_ofs.bombs_display + spr_bomb->h;

		labels.x.next_life = pos_lives - spr_life->w * 0.5;
		labels.x.next_bomb = pos_bombs - spr_bomb->w * 0.5;

		draw_fragments(&(DrawFragmentsParams) {
			.fill = spr_life,
			.pos = { pos_lives, labels.y.lives + labels.y_ofs.lives_display },
			.origin_offset = { 0, 0 },
			.limits = { PLR_MAX_LIVES, PLR_MAX_LIFE_FRAGMENTS },
			.filled = { global.plr.lives, global.plr.life_fragments },
			.alpha = 1,
			.spacing = spacing,
			.color = {
				.fill = color_mul(RGBA(1, 1, 1, 1), &labels.lb_baseclr),
				.back = RGBA(0, 0, 0, 0.5),
				.frag = RGBA(0.5, 0.5, 0.6, 0.5),
			}
		});

		draw_fragments(&(DrawFragmentsParams) {
			.fill = spr_bomb,
			.pos = { pos_bombs, labels.y.bombs + labels.y_ofs.bombs_display },
			.origin_offset = { 0, 0.05 },
			.limits = { PLR_MAX_BOMBS, PLR_MAX_BOMB_FRAGMENTS },
			.filled = { global.plr.bombs, global.plr.bomb_fragments },
			.alpha = 1,
			.spacing = spacing,
			.color = {
				.fill = color_mul(RGBA(1, 1, 1, 1), &labels.lb_baseclr),
				.back = color_mul(RGBA(0, 0, 0, 0.5), &labels.lb_baseclr),
				.frag = color_mul(RGBA(0.5, 0.5, 0.6, 0.5), &labels.lb_baseclr),
			}
		});

		r_mat_mv_pop();
	}

	// Difficulty indicator
	r_draw_sprite(&(SpriteParams) {
		.sprite = difficulty_sprite_name(global.diff),
		.pos = { HUD_EFFECTIVE_WIDTH * 0.5, 400 },
		.scale.both = 0.6,
		.shader = "sprite_default",
	});

	// Power/Item/Voltage icons
	r_mat_mv_push();
	r_mat_mv_translate(HUD_X_SECONDARY_OFS_ICON, font_get_descent(res_font("standard")) * 0.5 - 1, 0);

	r_draw_sprite(&(SpriteParams) {
		.pos = { 2, labels.y.power + 2 },
		.sprite = "item/power",
		.shader = "sprite_default",
		.color = RGBA(0, 0, 0, 0.5),
	});

	r_draw_sprite(&(SpriteParams) {
		.pos = { 0, labels.y.power },
		.sprite = "item/power",
		.shader = "sprite_default",
	});

	r_draw_sprite(&(SpriteParams) {
		.pos = { 2, labels.y.value + 2 },
		.sprite = "item/point",
		.shader = "sprite_default",
		.color = RGBA(0, 0, 0, 0.5),
	});

	r_draw_sprite(&(SpriteParams) {
		.pos = { 0, labels.y.value },
		.sprite = "item/point",
		.shader = "sprite_default",
	});

	r_draw_sprite(&(SpriteParams) {
		.pos = { 2, labels.y.voltage + 2 },
		.sprite = "item/voltage",
		.shader = "sprite_default",
		.color = RGBA(0, 0, 0, 0.5),
	});

	r_draw_sprite(&(SpriteParams) {
		.pos = { 0, labels.y.voltage },
		.sprite = "item/voltage",
		.shader = "sprite_default",
	});

	r_mat_mv_pop();
	stage_draw_hud_text(&labels);

	// Extra Spell indicator
	if(extraspell_alpha > 0) {
		float s2 = fmax(0, swing(extraspell_alpha, 3));
		r_state_push();
		r_shader("text_default");
		r_mat_mv_push();
		r_mat_mv_translate(lerp(-HUD_X_OFFSET - HUD_X_PADDING, HUD_EFFECTIVE_WIDTH * 0.5, pow(2*extraspell_fadein-1, 2)), 128, 0);
		r_mat_mv_rotate((360 * (1-s2) - 25) * DEG2RAD, 0, 0, 1);
		r_mat_mv_scale(s2, s2, 0);
		r_color(RGBA_MUL_ALPHA(0.3, 0.6, 0.7, 0.7 * extraspell_alpha));

		Font *font = res_font("big");

		// TODO: replace this with a shader
		text_draw("Voltage        \n    Overdrive!", &(TextParams) { .pos = {  1,  1 }, .font_ptr = font, .align = ALIGN_CENTER });
		text_draw("Voltage        \n    Overdrive!", &(TextParams) { .pos = { -1, -1 }, .font_ptr = font, .align = ALIGN_CENTER });
		r_color4(extraspell_alpha, extraspell_alpha, extraspell_alpha, extraspell_alpha);
		text_draw("Voltage        \n    Overdrive!", &(TextParams) { .pos = {  0,  0 }, .font_ptr = font, .align = ALIGN_CENTER });

		r_mat_mv_pop();
		r_state_pop();
	}

	if(stagedraw.framerate_graphs) {
		stage_draw_framerate_graphs(0, 360, HUD_EFFECTIVE_WIDTH, 30);
	}

	r_mat_mv_pop();

	// Boss indicator ("Enemy")
	if(global.boss) {
		float red = 0.5*exp(-0.5*(global.frames-global.boss->lastdamageframe)); // hit indicator
		if(red > 1)
			red = 0;

		r_draw_sprite(&(SpriteParams) {
			.sprite = "boss_indicator",
			.shader = "sprite_default",
			.pos = { VIEWPORT_X+creal(global.boss->pos), 590 },
			.color = RGBA(1 - red, 1 - red, 1 - red, 1 - red),
		});
	}
}

void stage_display_clear_screen(const StageClearBonus *bonus) {
	StageTextTable tbl;
	stagetext_begin_table(&tbl, bonus->all_clear ? "All Clear!" : "Stage Clear!", RGB(1, 1, 1), RGB(1, 1, 1), VIEWPORT_W/2,
		20, 5184000, 60, 60);
	stagetext_table_add_numeric_nonzero(&tbl, "Clear bonus", bonus->base);
	stagetext_table_add_numeric_nonzero(&tbl, "Life bonus", bonus->lives);
	stagetext_table_add_numeric_nonzero(&tbl, "Voltage bonus", bonus->voltage);
	stagetext_table_add_numeric_nonzero(&tbl, "Graze bonus", bonus->graze);
	stagetext_table_add_separator(&tbl);
	stagetext_table_add_numeric(&tbl, "Total", bonus->total);
	stagetext_end_table(&tbl);

	stagetext_add(
		"Press Fire to continue",
		VIEWPORT_W/2 + VIEWPORT_H*0.7*I,
		ALIGN_CENTER,
		res_font("standard"),
		RGB(1, 0.5, 0),
		tbl.delay,
		tbl.lifetime,
		tbl.fadeintime,
		tbl.fadeouttime
	);

	stagedraw.clear_screen.target_alpha = 1;
}

StageDrawEvents *stage_get_draw_events(void) {
	return &stagedraw.events;
}
