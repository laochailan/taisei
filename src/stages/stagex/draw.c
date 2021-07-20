/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "draw.h"
#include "stageutils.h"
#include "random.h"
#include "renderer/api.h"
#include "global.h"
#include "stage.h"
#include "stagedraw.h"
#include "util/glm.h"
#include "coroutine.h"

static StageXDrawData *draw_data;

/*
 * BEGIN utils
 */

StageXDrawData *stagex_get_draw_data(void) {
	return draw_data;
}

bool stagex_drawing_into_glitchmask(void) {
	return r_framebuffer_current() == draw_data->fb.glitch_mask;
}

static float fsplitmix(uint32_t *state) {
	return (float)(splitmix32(state) / (double)UINT32_MAX);
}

static bool should_draw_tower(void) {
	return draw_data->tower_global_dissolution < 0.999;
}

static bool should_draw_tower_postprocess(void) {
	return should_draw_tower() && draw_data->tower_partial_dissolution > 0;
}

/*
 * END utils
 */

/*
 * BEGIN background render
 */


static void stagex_bg_setup_pbr_lighting(void) {
	Camera3D *cam = &stage_3d_context.cam;
	StageXDrawData *draw_data = stagex_get_draw_data();

	float p = 300;
	float f = 10000 * draw_data->fog.opacity;

	Color c = draw_data->fog.color;
	color_mul_scalar(&c, f);

	PointLight3D lights[] = {
		{ { cam->pos[0], cam->pos[1], cam->pos[2] }, { 0.9*p, 0.8*p, p } },
		{ { cam->pos[0], cam->pos[1], cam->pos[2] + 60}, { c.r, c.g, c.b } },
	};

	camera3d_set_point_light_uniforms(cam, ARRAY_SIZE(lights), lights);

	float a = 0;
	r_uniform_vec3("ambient_color", a, a, a);
}


static void bg_begin_3d(void) {
	r_mat_proj_perspective(STAGE3D_DEFAULT_FOVY, STAGE3D_DEFAULT_ASPECT, 1700, STAGEX_BG_MAX_RANGE);
	stage_3d_context.cam.rot.pitch += draw_data->plr_pitch;
	stage_3d_context.cam.rot.yaw += draw_data->plr_yaw;
}

static void bg_end_3d(void) {
	stage_3d_context.cam.rot.pitch -= draw_data->plr_pitch;
	stage_3d_context.cam.rot.yaw -= draw_data->plr_yaw;
}

static uint bg_tower_pos(Stage3D *s3d, vec3 pos, float maxrange) {
#if 0
	vec3 p = { 0, 0, 0 };
	vec3 r = { 0, 0, STAGEX_BG_SEGMENT_PERIOD };

	maxrange /= 2;

	uint pnum = linear3dpos(s3d, pos, maxrange, p, r);

	for(uint i = 0; i < pnum; ++i) {
		stage_3d_context.pos_buffer[i][2] -= maxrange - r[2];
	}

	return pnum;
#else
	vec3 p = {0, 0, 0};
	vec3 r = {0, 0, STAGEX_BG_SEGMENT_PERIOD};

	return linear3dpos(s3d, pos, maxrange, p, r);
#endif
}

static void draw_tower(vec3 pos) {
	r_state_push();
	r_mat_mv_push();
	r_mat_mv_translate(pos[0], pos[1], pos[2]);

	r_shader("pbr");
	stagex_bg_setup_pbr_lighting();

	r_uniform_float("metallic", 0);
	r_uniform_sampler("tex", "stage5/stairs_diffuse");
	r_uniform_sampler("roughness_map", "stage5/stairs_roughness");
	r_uniform_sampler("normal_map", "stage5/stairs_normal");
	r_uniform_sampler("ambient_map", "stage5/stairs_ambient");
	r_draw_model("stage5/stairs");

	r_uniform_sampler("tex", "stage5/wall_diffuse");
	r_uniform_sampler("roughness_map", "stage5/wall_roughness");
	r_uniform_sampler("normal_map", "stage5/wall_normal");
	r_uniform_sampler("ambient_map", "stage5/wall_ambient");
	r_draw_model("stage5/wall");

	r_uniform_float("metallic", 1);
	r_uniform_vec3("ambient_color",0,0,0);
	r_uniform_sampler("tex", "stage5/metal_diffuse");
	r_uniform_sampler("roughness_map", "stage5/metal_roughness");
	r_uniform_sampler("normal_map", "stage5/metal_normal");
	r_draw_model("stage5/metal");

	r_mat_mv_pop();
	r_state_pop();
}

static void bg_tower_draw(vec3 pos, bool is_mask) {
	r_state_push();
	r_mat_mv_push();
// 	r_mat_mv_translate(pos[0], pos[1], pos[2]);
// 	r_mat_mv_scale(300,300,300);

	if(is_mask) {
		uint32_t tmp = float_to_bits(pos[2]);
		float tex_ofs_x = fsplitmix(&tmp);
		float tex_ofs_y = fsplitmix(&tmp);
		float tex_rot = fsplitmix(&tmp);
		float tex_scale_x = 0.7 + fsplitmix(&tmp) * 0.4;
		float tex_scale_y = 0.7 + fsplitmix(&tmp) * 0.4;
		float phase = fsplitmix(&tmp);
		r_mat_tex_push();
		r_mat_tex_translate(tex_ofs_x, tex_ofs_y, 0);
		r_mat_tex_rotate(tex_rot * M_PI * 2, 0, 0, 1);
		r_mat_tex_scale(tex_scale_x, tex_scale_y, 0);
		r_shader("extra_tower_mask");
		r_uniform_sampler("tex", "cell_noise");
		r_uniform_sampler("tex2", "stagex/dissolve_mask");
		r_uniform_float("time", global.frames/60.0f + phase * M_PI * 2.0f);
		r_uniform_float("dissolve", draw_data->tower_partial_dissolution * draw_data->tower_partial_dissolution);
		r_draw_model("tower_alt_uv");
		r_mat_tex_pop();
	} else {
// 		r_shader("tower_light");
// 		r_uniform_sampler("tex", "stage5/tower");
// 		r_uniform_vec3("lightvec", 0, 0, 0);
// 		r_uniform_vec4("color", 0.1, 0.1, 0.5, 1);
// 		r_uniform_float("strength", 0.5);
// 		r_draw_model("tower");
		draw_tower(pos);
	}

	r_mat_mv_pop();
	r_state_pop();
}

static void bg_tower_draw_solid(vec3 pos) {
	bg_tower_draw(pos, false);
}

static void bg_tower_draw_mask(vec3 pos) {
	bg_tower_draw(pos, true);
}

static void set_bg_uniforms(void) {
	r_uniform_sampler("background_tex", "stagex/bg");
	r_uniform_sampler("background_binary_tex", "stagex/bg_binary");
	r_uniform_sampler("code_tex", "stagex/code");
	r_uniform_vec4("code_tex_params",
		draw_data->codetex_aspect[0],
		draw_data->codetex_aspect[1],
		draw_data->codetex_num_segments,
		1.0f / draw_data->codetex_num_segments
	);
	r_uniform_vec2("dissolve",
		1 - draw_data->tower_global_dissolution,
		(1 - draw_data->tower_global_dissolution) * (1 - draw_data->tower_global_dissolution)
	);
	r_uniform_float("time", global.frames / 60.0f);
}

void stagex_draw_background(void) {
	if(should_draw_tower()) {
		bg_begin_3d();

		Stage3DSegment segs[] = {
			{ bg_tower_draw_solid, bg_tower_pos },
		};

		stage3d_draw(&stage_3d_context, STAGEX_BG_MAX_RANGE, ARRAY_SIZE(segs), segs);
		bg_end_3d();
	} else {
		r_state_push();
		r_mat_proj_push_ortho(VIEWPORT_W, VIEWPORT_H);
		r_mat_mv_push_identity();
		r_disable(RCAP_DEPTH_TEST);
		r_shader("extra_bg");
		set_bg_uniforms();
		r_mat_mv_scale(VIEWPORT_W, VIEWPORT_H, 1);
		r_mat_mv_translate(0.5, 0.5, 0);
		r_draw_quad();
		r_mat_mv_pop();
		r_mat_proj_pop();
		r_state_pop();
	}
}

/*
 * END background render
 */

/*
 * BEGIN background effects
 */

static void render_tower_mask(Framebuffer *fb) {
	r_state_push();
	r_mat_proj_push();
	bg_begin_3d();
	r_enable(RCAP_DEPTH_TEST);

	r_framebuffer(fb);
	r_clear(CLEAR_ALL, RGBA(1, 1, 1, draw_data->tower_global_dissolution), 1);

	r_mat_mv_push();
	stage3d_apply_transforms(&stage_3d_context, *r_mat_mv_current_ptr());
	stage3d_draw_segment(&stage_3d_context, bg_tower_pos, bg_tower_draw_mask, STAGEX_BG_MAX_RANGE);
	r_mat_mv_pop();

	bg_end_3d();
	r_mat_proj_pop();
	r_state_pop();
}

static bool bg_effect_tower_mask(Framebuffer *fb) {
	if(!should_draw_tower_postprocess()) {
		return false;
	}

	Framebuffer *mask_fb = draw_data->fb.tower_mask;
	render_tower_mask(mask_fb);

	r_state_push();
	r_enable(RCAP_DEPTH_TEST);
	r_enable(RCAP_DEPTH_WRITE);
	r_depth_func(DEPTH_ALWAYS);
	r_shader("extra_tower_apply_mask");
	r_uniform_sampler("mask_tex", r_framebuffer_get_attachment(mask_fb, FRAMEBUFFER_ATTACH_COLOR0));
	r_uniform_sampler("depth_tex", r_framebuffer_get_attachment(fb, FRAMEBUFFER_ATTACH_DEPTH));
	set_bg_uniforms();
	draw_framebuffer_tex(fb, VIEWPORT_W, VIEWPORT_H);
	r_state_pop();

	return true;
}

static bool bg_effect_copy_depth(Framebuffer *fb) {
	if(should_draw_tower_postprocess()) {
		r_state_push();
		r_enable(RCAP_DEPTH_TEST);
		r_depth_func(DEPTH_ALWAYS);
		r_blend(BLEND_NONE);
		r_shader("copy_depth");
		draw_framebuffer_attachment(fb, VIEWPORT_W, VIEWPORT_H, FRAMEBUFFER_ATTACH_DEPTH);
		r_state_pop();
	}

	return false;
}

static bool bg_effect_fog(Framebuffer *fb) {
	if(!should_draw_tower()) {
		return false;
	}

	Color c = draw_data->fog.color;
	color_mul_scalar(&c, draw_data->fog.opacity);

	r_shader("zbuf_fog");
	r_uniform_sampler("depth", r_framebuffer_get_attachment(fb, FRAMEBUFFER_ATTACH_DEPTH));
	r_uniform_vec4_rgba("fog_color", &c);
	r_uniform_float("start", 0.0);
	r_uniform_float("end", 1.0);
	r_uniform_float("exponent", draw_data->fog.exponent);
	r_uniform_float("sphereness", 0.0);
	draw_framebuffer_tex(fb, VIEWPORT_W, VIEWPORT_H);
	r_shader_standard();
	return true;
}

ShaderRule stagex_bg_effects[] = {
// 	bg_effect_tower_mask,
// 	bg_effect_copy_depth,
	bg_effect_fog,
	NULL
};

/*
 * END background effects
 */

/*
 * BEGIN postprocess
 */

static bool glitchmask_draw_predicate(EntityInterface *ent) {
	switch(ent->type) {
		case ENT_TYPE_ID(Enemy):
		case ENT_TYPE_ID(YumemiSlave):
// 		case ENT_TYPE_ID(BossShield):
			return true;

		default:
			return false;
	}
}

static void render_glitch_mask(Framebuffer *fb) {
	r_state_push();
	r_framebuffer(fb);
	r_clear(CLEAR_ALL, RGBA(0, 0, 0, 0), 1);
	r_shader("sprite_default");
	ent_draw(glitchmask_draw_predicate);
	r_state_pop();
}

static bool postprocess_glitch(Framebuffer *fb) {
	Framebuffer *glitch_mask = draw_data->fb.glitch_mask;
	render_glitch_mask(glitch_mask);

	vec3 t = { 0.143133f, 0.53434f, 0.25332f };
	glm_vec3_adds(t, global.frames / 60.0f, t);

	r_state_push();
	r_clear(CLEAR_ALL, RGBA(0, 0, 0, 1), 1);
	r_blend(BLEND_NONE);
	r_shader("extra_glitch");
	r_uniform_vec3_vec("times", t);
	r_uniform_sampler("mask", r_framebuffer_get_attachment(glitch_mask, FRAMEBUFFER_ATTACH_COLOR0));
	draw_framebuffer_tex(fb, VIEWPORT_W, VIEWPORT_H);
	/*
	r_blend(BLEND_PREMUL_ALPHA);
	r_color4(100, 100, 100, 0);
	draw_framebuffer_tex(glitch_mask, VIEWPORT_W, VIEWPORT_H);
	*/
	r_state_pop();

	return true;
}

ShaderRule stagex_postprocess_effects[] = {
	postprocess_glitch,
	NULL
};

/*
 * END postprocess
 */

/*
 * BEGIN camera update loop
 */

TASK(update_camera, NO_ARGS) {
	for(;;) {
		stage3d_update(&stage_3d_context);
		stage_3d_context.cam.rot.roll += draw_data->tower_spin;
		float yaw   = 10.0f * (crealf(global.plr.pos) / VIEWPORT_W - 0.5f) * draw_data->plr_influence;
		float pitch = 10.0f * (cimagf(global.plr.pos) / VIEWPORT_H - 0.5f) * draw_data->plr_influence;
		fapproach_asymptotic_p(&draw_data->plr_yaw,   yaw,   0.03, 1e-4);
		fapproach_asymptotic_p(&draw_data->plr_pitch, pitch, 0.03, 1e-4);
		YIELD;
	}
}

/*
 * END camera update loop
 */

/*
 * BEGIN init/shutdown
 */

static void init_tower_mask_fb(void) {
	FBAttachmentConfig a[2] = { 0 };
	a[0].attachment = FRAMEBUFFER_ATTACH_COLOR0;
	a[0].tex_params.type = TEX_TYPE_RGBA_16;
	a[0].tex_params.filter.min = TEX_FILTER_LINEAR;
	a[0].tex_params.filter.mag = TEX_FILTER_LINEAR;
	a[0].tex_params.width = VIEWPORT_W;
	a[0].tex_params.height = VIEWPORT_H;
	a[0].tex_params.wrap.s = TEX_WRAP_MIRROR;
	a[0].tex_params.wrap.t = TEX_WRAP_MIRROR;

	a[1].attachment = FRAMEBUFFER_ATTACH_DEPTH;
	a[1].tex_params.type = TEX_TYPE_DEPTH;
	a[1].tex_params.filter.min = TEX_FILTER_NEAREST;
	a[1].tex_params.filter.mag = TEX_FILTER_NEAREST;
	a[1].tex_params.width = VIEWPORT_W;
	a[1].tex_params.height = VIEWPORT_H;
	a[1].tex_params.wrap.s = TEX_WRAP_MIRROR;
	a[1].tex_params.wrap.t = TEX_WRAP_MIRROR;

	draw_data->fb.tower_mask = stage_add_background_framebuffer("Tower mask", 0.25, 0.5, ARRAY_SIZE(a), a);
}

static void init_glitch_mask_fb(void) {
	FBAttachmentConfig a[1] = { 0 };
	a[0].attachment = FRAMEBUFFER_ATTACH_COLOR0;
	a[0].tex_params.type = TEX_TYPE_RGB_8;
	a[0].tex_params.filter.min = TEX_FILTER_NEAREST;
	a[0].tex_params.filter.mag = TEX_FILTER_NEAREST;
	a[0].tex_params.width = VIEWPORT_W / 30;
	a[0].tex_params.height = VIEWPORT_H / 20;
	a[0].tex_params.wrap.s = TEX_WRAP_REPEAT;
	a[0].tex_params.wrap.t = TEX_WRAP_REPEAT;

	draw_data->fb.glitch_mask = stage_add_static_framebuffer("Glitch mask", ARRAY_SIZE(a), a);
}

static void init_spellbg_fb(void) {
	FBAttachmentConfig a[1] = { 0 };
	a[0].attachment = FRAMEBUFFER_ATTACH_COLOR0;
	a[0].tex_params.type = TEX_TYPE_RGB_8;
	a[0].tex_params.filter.min = TEX_FILTER_LINEAR;
	a[0].tex_params.filter.mag = TEX_FILTER_LINEAR;
	a[0].tex_params.width = VIEWPORT_W / 30;
	a[0].tex_params.height = VIEWPORT_H / 20;
	a[0].tex_params.wrap.s = TEX_WRAP_REPEAT;
	a[0].tex_params.wrap.t = TEX_WRAP_REPEAT;

	draw_data->fb.spell_background_lq = stage_add_background_framebuffer("Extra spell bg", 0.25, 0.5, ARRAY_SIZE(a), a);
}

void stagex_drawsys_init(void) {
	draw_data = calloc(1, sizeof(*draw_data));

	stage3d_init(&stage_3d_context, 16);
	stage_3d_context.cam.far = 128;

	init_tower_mask_fb();
	init_glitch_mask_fb();
	init_spellbg_fb();

	SDL_RWops *stream = vfs_open("res/gfx/stagex/code.num_slices", VFS_MODE_READ);

	if(!stream) {
		log_fatal("VFS error: %s", vfs_get_error());
	}

	char buf[32];
	SDL_RWgets(stream, buf, sizeof(buf));
	draw_data->codetex_num_segments = strtol(buf, NULL, 0);
	SDL_RWclose(stream);

	Texture *tex_code = res_texture("stagex/code");
	uint w, h;
	r_texture_get_size(tex_code, 0, &w, &h);

	// FIXME: this doesn't seem right, i don't know what the fuck i'm doing!
	float viewport_aspect = (float)VIEWPORT_W / (float)VIEWPORT_H;
	float seg_w = w / draw_data->codetex_num_segments;
	float seg_h = h;
	float seg_aspect = seg_w / seg_h;
	draw_data->codetex_aspect[0] = 1;
	draw_data->codetex_aspect[1] = 0.5/(seg_aspect/viewport_aspect);

	INVOKE_TASK(update_camera);
}

void stagex_drawsys_shutdown(void) {
	free(draw_data);
	draw_data = NULL;
	stage3d_shutdown(&stage_3d_context);
}

/*
 * END init/shutdown
 */
