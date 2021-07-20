/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "credits.h"
#include "global.h"
#include "stages/stage6/draw.h"
#include "video.h"
#include "resource/model.h"
#include "renderer/api.h"
#include "util/fbmgr.h"
#include "util/glm.h"
#include "dynarray.h"

typedef struct CreditsEntry {
	char **data;
	int lines;
	int time;
} CreditsEntry;

static struct {
	DYNAMIC_ARRAY(CreditsEntry) entries;
	float panelalpha;
	int end;
	bool skipable;
	CallChain cc;

	ManagedFramebufferGroup *mfb_group;
	Framebuffer *fb;

	struct {
		PBRModel tower;
	} models;

	Texture *env_map;
} credits;

#define CREDITS_ENTRY_FADEIN 200.0
#define CREDITS_ENTRY_FADEOUT 100.0

#define CREDITS_FADEOUT 180

// ideally, this should be timed so that:
//      (ENTRY_TIME * numEntries) + (HEADER_TIME * numHeaders) ~= 7190
#define ENTRY_TIME 412
#define HEADER_TIME 300
#define YUKKURI_TIME 200

static void credits_add(char *data, int time);

static void credits_fill(void) {
	// In case the shortened URLs break,
	// Tuck V's YouTube: https://www.youtube.com/channel/UCaw73cuHLnFCSpjOtt_9pyg
	// InsideI's bandcamp: https://vnutriya.bandcamp.com/

	dynarray_ensure_capacity(&credits.entries, 24);

	credits_add("Taisei Project\nbrought to you by…", HEADER_TIME);

	credits_add((
		"laochailan\n"
		"Lukas Weber\n"
		"laochailan@web.de\n\n"
		"Programming, game design,"
		"\ngraphics"
	), ENTRY_TIME);

	credits_add((
		"Akari\n"
		"Andrei Alexeyev\n"
		"akari@taisei-project.org\n\n"
		"Programming, game design"
	), ENTRY_TIME);

	credits_add((
		"Tuck V\n"
		"Discord: @Tuck#1679\n"
		"YouTube: https://is.gd/exafez\n\n"
		"Original soundtrack"
	), ENTRY_TIME);

	credits_add((
		"afensorm\n"
		"https://gensokyo.social/@afensorm\n"
		"https://pixiv.me/afens\n\n"
		"Character art"
	), ENTRY_TIME);

	credits_add((
		"mia\n"
		"Mia Herkt\n"
		"mia@hong-mailing.de\n\n"
		"Hosting, packaging, editing,\n"
		"spiritual guidance"
	), ENTRY_TIME);

	credits_add((
		"Alice D\n"
		"https://twitter.com/AmyZenunim\n"
		"https://github.com/starwitch\n\n"
		"Lead story writer\n"
		"macOS QA + debugging"
	), ENTRY_TIME);

	credits_add((
		"Adam\n"
		"https://twitter.com/adam_dnh\n\n"
		"Dialogue writing (Iku, Elly)"
	), ENTRY_TIME);

	credits_add((
		"Haru\n"
		"https://venusers.tumblr.com/\n"
		"https://twitter.com/violet_fantasia\n\n"
		"Dialogue writing (Hina, Kurumi)"
	), ENTRY_TIME);

	credits_add((
		"makise-homura\n"
		"Igor Molchanov\n"
		"akemi_homura@kurisa.ch\n\n"
		"Code contributions\n"
		"Elbrus compatible™"
	), ENTRY_TIME);

	credits_add((
		"aiju\n"
		"Julius Schmidt\n"
		"http://aiju.de/\n\n"
		"I don't remember\n"
		"what this guy did"
	), ENTRY_TIME);

	credits_add("Special Thanks", HEADER_TIME);

	credits_add((
		"ZUN\n"
		"for Tōhō Project\n"
		"http://www16.big.or.jp/~zun/"
	), ENTRY_TIME);

	credits_add((
		"InsideI\n"
		"Mikhail Novik\n"
		"Bandcamp: https://is.gd/owojix\n\n"
		"Various sound effects"
	), ENTRY_TIME);

	credits_add((
		"Free Software\n"
		"Simple DirectMedia Layer\n"  "https://libsdl.org/\n\n"
		"libpng\n"                    "http://libpng.org/\n\n"
		"libwebp\n"                   "https://git.io/WebP\n\n"
		"FreeType\n"                  "https://freetype.org/\n\n"
		"Ogg Opus\n"                  "https://www.opus-codec.org/"
	), ENTRY_TIME);

	credits_add((
		"\n"
		"zlib\n"                      "https://zlib.net/\n\n"
		"libzip\n"                    "https://libzip.org/\n\n"
		"Meson build system\n"        "https://mesonbuild.com/\n\n"
		"Krita\n"                     "https://krita.org/\n\n"
		"and many other projects"
	), ENTRY_TIME);

	credits_add((
		"…and You!\n"
		"for playing"
	), ENTRY_TIME);

	credits_add((
		"Visit Us\n"
		"https://taisei-project.org/\n\n"
		"And join our IRC channel\n"
		"#taisei-project at irc.freenode.net\n\n"
		"Or our Discord server\n"
		"https://discord.gg/JEHCMzW"
	), ENTRY_TIME);

	// Yukkuri Kyouko!
	credits_add("*\nAnd don't forget to take it easy!", YUKKURI_TIME);
}

static void credits_add(char *data, int time) {
	CreditsEntry *e;
	char *c, buf[256];
	int l = 0, i = 0;

	assert(time > CREDITS_ENTRY_FADEOUT);

	e = dynarray_append(&credits.entries);
	e->time = time - CREDITS_ENTRY_FADEOUT;
	e->lines = 1;

	for(c = data; *c; ++c)
		if(*c == '\n') e->lines++;
	e->data = malloc(e->lines * sizeof(char*));

	for(c = data; *c; ++c) {
		if(*c == '\n') {
			buf[i] = 0;
			e->data[l] = malloc(strlen(buf) + 1);
			strcpy(e->data[l], buf);
			i = 0;
			++l;
		} else {
			buf[i++] = *c;
		}
	}

	buf[i] = 0;
	e->data[l] = malloc(strlen(buf) + 1);
	strcpy(e->data[l], buf);
	credits.end += time;
}

static void credits_skysphere_draw(vec3 pos) {
	r_state_push();
	r_disable(RCAP_DEPTH_TEST);
	r_cull(CULL_FRONT);
	r_shader("stage6_sky");
	r_uniform_sampler("skybox", "stage6/sky");

	r_mat_mv_push();
	r_mat_mv_translate_v(stage_3d_context.cam.pos);
	r_mat_mv_scale(50, 50, 50);
	r_draw_model("cube");
	r_mat_mv_pop();
	r_enable(RCAP_DEPTH_TEST);
	r_state_pop();
}

static void credits_bg_setup_pbr_lighting(Camera3D *cam) {
	PointLight3D lights[] = {
		{ {0, 100, 100}, { 1000, 1000, 1000 } },
	};

	camera3d_set_point_light_uniforms(cam, ARRAY_SIZE(lights), lights);
}

static void credits_bg_setup_pbr_env(Camera3D *cam, PBREnvironment *env) {
	credits_bg_setup_pbr_lighting(cam);
	glm_vec3_broadcast(1.0f, env->ambient_color);
	camera3d_apply_inverse_transforms(cam, env->cam_inverse_transform);
	env->environment_map = credits.env_map;
}

static void credits_towerwall_draw(vec3 pos) {
	r_state_push();
	r_shader("pbr");
	r_enable(RCAP_DEPTH_TEST);

	r_mat_mv_push();
	r_mat_mv_translate_v(pos);

	PBREnvironment env = { 0 };
	credits_bg_setup_pbr_env(&stage_3d_context.cam, &env);

	pbr_draw_model(&credits.models.tower, &env);

	r_shader("envmap_reflect");
	r_uniform_sampler("envmap", env.environment_map);
	r_uniform_mat4("inv_camera_transform", env.cam_inverse_transform);
	r_draw_model("credits/metal_columns");

	r_mat_mv_pop();
	r_state_pop();
}

static void resize_fb(void *userdata, IntExtent *out_dimensions, FloatRect *out_viewport) {
	float w, h;
	video_get_viewport_size(&w, &h);

	out_dimensions->w = w;
	out_dimensions->h = h;

	out_viewport->w = w;
	out_viewport->h = h;
	out_viewport->x = 0;
	out_viewport->y = 0;
}

static void credits_init(void) {
	memset(&credits, 0, sizeof(credits));
	stage3d_init(&stage_3d_context, 32);

	FBAttachmentConfig a[2];
	memset(a, 0, sizeof(a));

	for(int i = 0; i < ARRAY_SIZE(a); i++) {
		a[i].tex_params.filter.min = TEX_FILTER_LINEAR;
		a[i].tex_params.filter.mag = TEX_FILTER_LINEAR;
		a[i].tex_params.wrap.s = TEX_WRAP_MIRROR;
		a[i].tex_params.wrap.s = TEX_WRAP_MIRROR;
	}

	a[0].attachment = FRAMEBUFFER_ATTACH_COLOR0;
	a[0].tex_params.type = TEX_TYPE_RGBA_8;
	a[1].attachment = FRAMEBUFFER_ATTACH_DEPTH;
	a[1].tex_params.type = TEX_TYPE_DEPTH;

	FramebufferConfig fbconf = { 0 };
	fbconf.attachments = a;
	fbconf.num_attachments = ARRAY_SIZE(a);
	fbconf.resize_strategy.resize_func = resize_fb;

	credits.mfb_group = fbmgr_group_create();
	credits.fb = fbmgr_group_framebuffer_create(credits.mfb_group, "BG", &fbconf);

	stage_3d_context.cam.far = 1000;

	stage_3d_context.cam.pos[0] = 0;
	stage_3d_context.cam.pos[1] = -19;
	stage_3d_context.cam.vel[2] = -0.5;
	stage_3d_context.cam.rot.v[0] = 30;
	stage_3d_context.cam.rot.v[2] = -20;

	global.frames = 0;

	credits.env_map = res_texture("stage6/sky");
	pbr_load_model(&credits.models.tower, "credits/tower", "credits/tower");

	credits_fill();
	credits.end += 200 + CREDITS_ENTRY_FADEOUT;

	// Should be >1, because if we get here, that means we have achieved an
	// ending just now, which this counter includes.
	// That is unless we're in `taisei --credits`. But in that case, skipping is
	// presumably not desired anyway.
	credits.skipable = progress_times_any_ending_achieved() > 1;

	audio_bgm_play(res_bgm("credits"), false, 0, 0);
}

static double entry_height(CreditsEntry *e, double *head, double *body) {
	double total = *head = *body = 0;

	if(!e->lines) {
		return total;
	}

	if(e->lines > 0) {
		if(*(e->data[0]) == '*') {
			total += *head = sprite_padded_height(res_sprite("kyoukkuri"));
		} else {
			total += *head = font_get_lineskip(res_font("big"));
		}

		if(e->lines > 1) {
			total += *body += (e->lines - 0.5) * font_get_lineskip(res_font("standard"));
		}
	}

	return total;
}

static float yukkuri_jump(float t) {
	float k = 0.2;
	float l = 1 - k;

	if(t > 1 || t < 0) {
		return 0;
	}

	if(t > k) {
		t = (t - k) / l;
		float b = glm_ease_bounce_out(t);
		float e = glm_ease_sine_out(b);
		return 1 - lerp(b, e, glm_ease_sine_out(t));
	}

	return glm_ease_quad_out(t / k);
}

static void credits_draw_entry(CreditsEntry *e) {
	int time = global.frames - 200;
	float fadein = 1, fadeout = 1;

	for(CreditsEntry *o = credits.entries.data; o != e; ++o) {
		time -= o->time + CREDITS_ENTRY_FADEOUT;
	}

	double h_total, h_head, h_body;
	h_total = entry_height(e, &h_head, &h_body);

	// random asspull approximation to make stuff not overlap too much
	int ofs = (1 - pow(1 - h_total / SCREEN_H, 2)) * SCREEN_H * 0.095;
	time -= ofs;

	if(time < 0) {
		return;
	}

	ofs *= 2;

	if(time <= CREDITS_ENTRY_FADEIN) {
		fadein = time / CREDITS_ENTRY_FADEIN;
	}

	if(time - e->time - CREDITS_ENTRY_FADEIN + ofs > 0) {
		fadeout = fmax(0, 1 - (time - e->time - CREDITS_ENTRY_FADEIN + ofs) / CREDITS_ENTRY_FADEOUT);
	}

	if(!fadein || !fadeout) {
		return;
	}

	Sprite *yukkuri_spr = NULL;

	if(*e->data[0] == '*') {
		yukkuri_spr = res_sprite("kyoukkuri");
	}

	r_state_push();
	r_mat_mv_push();

	if(fadein < 1) {
		r_mat_mv_translate(0, SCREEN_W * pow(1 - fadein,  2) *  0.5, 0);
	} else if(fadeout < 1) {
		r_mat_mv_translate(0, SCREEN_W * pow(1 - fadeout, 2) * -0.5, 0);
	}

	r_color(RGBA_MUL_ALPHA(1, 1, 1, fadein * fadeout));
	r_mat_mv_translate(0, h_body * -0.5, 0);

	for(int i = 0; i < e->lines; ++i) {
		if(yukkuri_spr && !i) {
			float t = ((global.frames) % 90) / 59.0;
			float elevation = yukkuri_jump(t);
			float squeeze = (elevation - yukkuri_jump(t - 0.03)) * 0.4;
			float halfheight = sprite_padded_height(yukkuri_spr) * 0.5;

			r_draw_sprite(&(SpriteParams) {
				.sprite_ptr = yukkuri_spr,
				.pos.y = -60 * elevation * fadein + halfheight * squeeze,
				.shader = "sprite_default",
				.scale.x = 1.0 - squeeze,
				.scale.y = 1.0 + squeeze,
			});

			r_mat_mv_translate(0, halfheight, 0);
		} else {
			Font *font = res_font(i ? "standard" : "big");
			r_shader("text_default");
			text_draw(e->data[i], &(TextParams) {
				.align = ALIGN_CENTER,
				.font_ptr = font,
			});
			r_shader_standard();
			r_mat_mv_translate(0, font_get_lineskip(font), 0);
		}
	}

	r_mat_mv_pop();
	r_state_pop();
}

static uint credits_towerwall_pos(Stage3D *s3d, vec3 pos, float maxrange) {
	vec3 p = {0, 0, 0};
	vec3 r = {0, 0, 25};

	uint num = linear3dpos(s3d, pos, maxrange, p, r);

	return num;
}

static uint credits_skysphere_pos(Stage3D *s3d, vec3 pos, float maxrange) {
	return single3dpos(s3d, pos, maxrange, s3d->cam.pos);
}

static void credits_draw(void) {
	r_clear(CLEAR_ALL, RGBA(0, 0, 0, 1), 1);
//	colorfill(1, 1, 1, 1); // don't use r_clear for this, it screws up letterboxing


	r_mat_mv_push();

	r_state_push();
	r_framebuffer(credits.fb);
	r_clear(CLEAR_ALL, RGBA(0, 0, 0, 1), 1);

	r_enable(RCAP_DEPTH_TEST);
	stage3d_draw(&stage_3d_context, 500, 2, (Stage3DSegment[]) { credits_skysphere_draw, credits_skysphere_pos, credits_towerwall_draw, credits_towerwall_pos });
	r_state_pop();
	draw_framebuffer_tex(credits.fb, SCREEN_W, SCREEN_H);

	r_mat_mv_pop();
	set_ortho(SCREEN_W, SCREEN_H);

	r_mat_mv_push();
	r_color4(0, 0, 0, credits.panelalpha * 0.7);
	r_mat_mv_translate(SCREEN_W/4*3, SCREEN_H/2, 0);
	r_mat_mv_scale(300, SCREEN_H, 1);
	r_shader_standard_notex();
	r_draw_quad();
	r_color4(1, 1, 1, 1);
	r_mat_mv_pop();

	r_mat_mv_push();
	r_mat_mv_translate(SCREEN_W/4*3, SCREEN_H/2, 0);

	r_shader_standard();

	dynarray_foreach_elem(&credits.entries, CreditsEntry *e, {
		credits_draw_entry(e);
	});

	r_mat_mv_pop();

	draw_transition();
}

static void credits_finish(void *arg) {
	credits.end = 0;
	set_transition(TransLoader, 0, FADE_TIME*10);
}

static void credits_process(void) {
	TIMER(&global.frames);

	stage3d_update(&stage_3d_context);

	//stage_3d_context.cam.pos[2] = 10-0.1*global.frames;
	//stage_3d_context.cam.pos[1] = 500 + 100 * psin(global.frames / 100.0) * psin(global.frames / 200.0 + M_PI);
	//stage_3d_context.cam.pos[0] = 25 * sin(global.frames / 75.7) * cos(global.frames / 99.3);

	FROM_TO(100, 200, 1)
		credits.panelalpha += 0.01;

	if(global.frames >= credits.end - CREDITS_ENTRY_FADEOUT) {
		credits.panelalpha -= 1 / 120.0;
	}

	if(global.frames == credits.end) {
		set_transition_callback(TransFadeWhite, CREDITS_FADEOUT, CREDITS_FADEOUT, credits_finish, NULL);
	}
}

static void credits_free(void) {
	fbmgr_group_destroy(credits.mfb_group);

	dynarray_foreach_elem(&credits.entries, CreditsEntry *e, {
		for(int i = 0; i < e->lines; ++i) {
			free(e->data[i]);
		}
		free(e->data);
	});

	dynarray_free_data(&credits.entries);
	stage3d_shutdown(&stage_3d_context);
}

void credits_preload(void) {
	preload_resource(RES_BGM, "credits", RESF_OPTIONAL);
	preload_resources(RES_SHADER_PROGRAM, RESF_DEFAULT,
		"pbr",
		"envmap_reflect",
		"stage6_sky",
	NULL);
	preload_resource(RES_SPRITE, "kyoukkuri", RESF_DEFAULT);
	preload_resources(RES_TEXTURE, RESF_DEFAULT,
		"loading",  // for transition
		"stage6/sky",
	NULL);
	preload_resources(RES_MATERIAL, RESF_DEFAULT,
		"credits/tower",
	NULL);
	preload_resources(RES_MODEL, RESF_DEFAULT,
		"credits/metal_columns",
		"credits/tower",
		"cube",
	NULL);
}

static LogicFrameAction credits_logic_frame(void *arg) {
	update_transition();
	events_poll(NULL, 0);
	credits_process();
	global.frames++;

	if(credits.end == 0) {
		return LFRAME_STOP;
	} else if(credits.skipable && gamekeypressed(KEY_SKIP)) {
		return LFRAME_SKIP;
	} else {
		return LFRAME_WAIT;
	}
}

static RenderFrameAction credits_render_frame(void *arg) {
	credits_draw();
	return RFRAME_SWAP;
}

static void credits_end_loop(void *ctx) {
	credits_free();
	progress_unlock_bgm("credits");
	run_call_chain(&credits.cc, NULL);
}

void credits_enter(CallChain next) {
	credits_preload();
	credits_init();
	credits.cc = next;
	eventloop_enter(&credits, credits_logic_frame, credits_render_frame, credits_end_loop, FPS);
}
