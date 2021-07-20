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
#include "global.h"
#include "util/glm.h"

static Stage4DrawData *stage4_draw_data;

Stage4DrawData *stage4_get_draw_data(void) {
	return NOT_NULL(stage4_draw_data);
}

static bool stage4_fog(Framebuffer *fb) {
	float f = 0;

	r_shader("zbuf_fog");
	r_uniform_sampler("depth", r_framebuffer_get_attachment(fb, FRAMEBUFFER_ATTACH_DEPTH));
	r_uniform_vec4("fog_color", 9.0*f+0.1, 0.0, 0.1-f, 1.0);
	r_uniform_float("start", 0.4);
	r_uniform_float("end", 1);
	r_uniform_float("exponent", 50.0);
	r_uniform_float("sphereness", 0);
	draw_framebuffer_tex(fb, VIEWPORT_W, VIEWPORT_H);
	r_shader_standard();

	return true;
}

static bool should_draw_water(void) {
	return stage_3d_context.cam.pos[1] < 0 && config_get_int(CONFIG_POSTPROCESS) > 1;
}

static bool stage4_water(Framebuffer *fb) {
	if(!should_draw_water()) {
		return false;
	}

	r_clear(CLEAR_COLOR, RGBA(0, 0, 0, 0), 1);
	r_mat_proj_push_perspective(stage_3d_context.cam.fovy, stage_3d_context.cam.aspect, stage_3d_context.cam.near, stage_3d_context.cam.far);
	r_state_push();
	r_mat_mv_push();
	stage3d_apply_transforms(&stage_3d_context, *r_mat_mv_current_ptr());
	r_mat_mv_translate(0, 3, -0.6);
	r_enable(RCAP_DEPTH_TEST);

	r_mat_mv_scale(20,-20,10);
	r_shader("ssr_water");
	r_uniform_sampler("depth", r_framebuffer_get_attachment(fb, FRAMEBUFFER_ATTACH_DEPTH));
	r_uniform_sampler("tex", r_framebuffer_get_attachment(fb, FRAMEBUFFER_ATTACH_COLOR0));
	r_uniform_float("time", global.frames * 0.002);
	r_uniform_vec2("wave_offset", -global.frames * 0.0005, 0);
	r_color4(0, 0.01, 0.01, 1);
	r_draw_quad();

	r_mat_mv_pop();
	r_state_pop();
	r_mat_proj_pop();

	return true;
}

static bool stage4_water_composite(Framebuffer *reflections) {
	if(!should_draw_water()) {
		return false;
	}

	r_shader("alpha_discard");
	r_blend(BLEND_NONE);
	r_uniform_float("threshold", 1);
	draw_framebuffer_tex(reflections, VIEWPORT_W, VIEWPORT_H);
	return true;
}

static void stage4_bg_setup_pbr_lighting_outdoors(Camera3D *cam) {
	PointLight3D lights[] = {
		{ { 0,  0, 0 },  { 3,  4,  5 } },
		{ { 0, 25, 3 },  { 4, 20, 22 } },
	};

	vec3 r;
	camera3d_unprojected_ray(cam, global.plr.pos, r);
	glm_vec3_scale(r, 4, r);
	glm_vec3_add(cam->pos, r, lights[0].pos);

	camera3d_set_point_light_uniforms(cam, ARRAY_SIZE(lights), lights);
}

static void stage4_bg_setup_pbr_env_outdoors(Camera3D *cam, PBREnvironment *env) {
	stage4_bg_setup_pbr_lighting_outdoors(cam);
	glm_vec3_broadcast(1.0f, env->ambient_color);
}

static void stage4_bg_setup_pbr_lighting_indoors(Camera3D *cam, vec3 pos) {
	float xoff = 3.0f;
	float zoff = 1.3f;

	PointLight3D lights[] = {
		{ { -xoff, pos[1],    pos[2]+zoff }, { 1, 20, 20 } },
		{ {  xoff, pos[1],    pos[2]+zoff }, { 1, 20, 20 } },
		{ { -xoff, pos[1]-10, pos[2]+zoff }, { 1, 20, 20 } },
		{ {  xoff, pos[1]-10, pos[2]+zoff }, { 1, 20, 20 } },
		{ { -xoff, pos[1]+10, pos[2]+zoff }, { 1, 20, 20 } },
		{ {  xoff, pos[1]+10, pos[2]+zoff }, { 1, 20, 20 } },
	};

	for(int i = 0; i < ARRAY_SIZE(lights); i++) {
		float t = global.frames * 0.02f;
		float mod1 = cosf(13095434.0f * lights[i].pos[1]);
		float mod2 = sinf(1242435.0f * lights[i].pos[0] * lights[i].pos[1]);

		float f = (
			sinf((1.0f + mod1) * t) +
			sinf((2.35f + mod2) * t + mod1) +
			sinf((3.1257f + mod1 * mod2) * t + mod2)
		) / 3.0f;

		glm_vec3_scale(lights[i].radiance, 0.6f + 0.4f * f, lights[i].radiance);
	}

	camera3d_set_point_light_uniforms(cam, ARRAY_SIZE(lights), lights);
}

static void stage4_bg_setup_pbr_env_indoors(Camera3D *cam, vec3 pos, PBREnvironment *env) {
	stage4_bg_setup_pbr_lighting_indoors(cam, pos);
	glm_vec3_copy(&stage4_draw_data->ambient_color.r, env->ambient_color);
	log_debug("%0.2f %0.2f %0.2f", env->ambient_color[0], env->ambient_color[1], env->ambient_color[2]);
}

static uint stage4_lake_pos(Stage3D *s3d, vec3 pos, float maxrange) {
	vec3 p = {0, 0, 0};
	return single3dpos(s3d, pos, maxrange, p);
}

static void stage4_lake_draw(vec3 pos) {
	r_state_push();
	r_mat_mv_push();
	r_mat_mv_translate_v(pos);

	r_shader("pbr");

	PBREnvironment env = { 0 };
	stage4_bg_setup_pbr_env_outdoors(&stage_3d_context.cam, &env);

	pbr_draw_model(&stage4_draw_data->models.ground, &env);
	pbr_draw_model(&stage4_draw_data->models.mansion, &env);

	r_mat_mv_pop();
	r_state_pop();
}

static uint stage4_corridor_pos(Stage3D *s3d, vec3 pos, float maxrange) {
	vec3 p = {0, 25, 3};
	vec3 r = {0, 10, 0};

	uint num = linear3dpos(s3d, pos, maxrange, p, r);

	for(uint i = 0; i < num; ++i) {
		if(s3d->pos_buffer[i][1] < p[1]) {
			s3d->pos_buffer[i][1] = -9000;
		}
	}

	return num;
}

static void stage4_corridor_draw(vec3 pos) {
	r_state_push();
	r_mat_mv_push();
	r_mat_mv_translate_v(pos);

	r_shader("pbr");

	PBREnvironment env = { 0 };
	stage4_bg_setup_pbr_env_indoors(&stage_3d_context.cam, pos, &env);

	pbr_draw_model(&stage4_draw_data->models.corridor, &env);

	r_mat_mv_pop();
	r_state_pop();
}

void stage4_draw(void) {
	Stage3DSegment segs[] = {
		{ stage4_lake_draw, stage4_lake_pos },
		{ stage4_corridor_draw, stage4_corridor_pos },
	};

	stage3d_draw(&stage_3d_context, 400, ARRAY_SIZE(segs), segs);
}

void stage4_drawsys_init(void) {
	stage4_draw_data = calloc(1, sizeof(*stage4_draw_data));
	stage3d_init(&stage_3d_context, 16);

	pbr_load_model(&stage4_draw_data->models.corridor, "stage4/corridor", "stage4/corridor");
	pbr_load_model(&stage4_draw_data->models.ground,   "stage4/ground",   "stage4/ground");
	pbr_load_model(&stage4_draw_data->models.mansion,  "stage4/mansion",  "stage4/mansion");
}

void stage4_drawsys_shutdown(void) {
	stage3d_shutdown(&stage_3d_context);
	free(stage4_draw_data);
	stage4_draw_data = NULL;
}

ShaderRule stage4_bg_effects[] = {
	stage4_water,
	stage4_water_composite,
	stage4_fog,
	NULL,
};
