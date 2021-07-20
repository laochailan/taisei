/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "draw.h"

#include "global.h"
#include "stageutils.h"
#include "util/glm.h"
#include "resource/model.h"

MODERNIZE_THIS_FILE_AND_REMOVE_ME

static Stage5DrawData *stage5_draw_data;

Stage5DrawData *stage5_get_draw_data(void) {
	return NOT_NULL(stage5_draw_data);
}

void stage5_drawsys_init(void) {
	stage5_draw_data = calloc(1, sizeof(*stage5_draw_data));
	stage3d_init(&stage_3d_context, 16);

	stage5_draw_data->stairs.light_pos = -200;

	stage5_draw_data->stairs.rad = 3.7;
	stage5_draw_data->stairs.zoffset = 3.2;
	stage5_draw_data->stairs.roffset = -210;

	pbr_load_model(&stage5_draw_data->models.metal,  "stage5/metal",  "stage5/metal");
	pbr_load_model(&stage5_draw_data->models.stairs, "stage5/stairs", "stage5/stairs");
	pbr_load_model(&stage5_draw_data->models.wall,   "stage5/wall",   "stage5/wall");

	stage5_draw_data->env_map = res_texture("stage5/envmap");
}

void stage5_drawsys_shutdown(void) {
	stage3d_shutdown(&stage_3d_context);
	free(stage5_draw_data);
	stage5_draw_data = NULL;
}

static uint stage5_stairs_pos(Stage3D *s3d, vec3 pos, float maxrange) {
	vec3 p = {0, 0, 0};
	vec3 r = {0, 0, 11.2};

	return linear3dpos(s3d, pos, maxrange, p, r);
}

static void stage5_bg_setup_pbr_lighting(Camera3D *cam) {
	PointLight3D lights[] = {
		{ { 1.2 * cam->pos[0], 1.2 * cam->pos[1], cam->pos[2] - 0.2 }, { 235*0.4, 104*0.4, 32*0.4 } },
		{ { 0, 0, cam->pos[2] - 1}, { 0.2, 0, 13.2 } },
		{ { 0, 0, cam->pos[2] - 6}, { 0.2, 0, 13.2 } },
		{ { 0, 0, cam->pos[2] + 100}, { 1000, 1000, 1000 } },
		// thunder lightning
		{ 0, cam->pos[1], cam->pos[2] + stage5_draw_data->stairs.light_pos, { 5000, 8000, 100000 } },
	};

	float light_strength = stage5_draw_data->stairs.light_strength;
	glm_vec3_scale(lights[3].radiance, 1+0.5*light_strength, lights[3].radiance);
	glm_vec3_scale(lights[4].radiance, light_strength, lights[4].radiance);

	camera3d_set_point_light_uniforms(cam, ARRAY_SIZE(lights), lights);
}

static void stage5_bg_setup_pbr_env(Camera3D *cam, PBREnvironment *env) {
	stage5_bg_setup_pbr_lighting(cam);
	glm_vec3_broadcast(1.0f + stage5_draw_data->stairs.light_strength, env->ambient_color);
	env->environment_map = stage5_draw_data->env_map;
	camera3d_apply_inverse_transforms(cam, env->cam_inverse_transform);
}

static void stage5_stairs_draw(vec3 pos) {
	r_state_push();
	r_mat_mv_push();
	r_mat_mv_translate(pos[0], pos[1], pos[2]);

	r_shader("pbr");

	PBREnvironment env = { 0 };
	stage5_bg_setup_pbr_env(&stage_3d_context.cam, &env);

	pbr_draw_model(&stage5_draw_data->models.stairs, &env);
	pbr_draw_model(&stage5_draw_data->models.wall, &env);
	pbr_draw_model(&stage5_draw_data->models.metal, &env);

	r_mat_mv_pop();
	r_state_pop();
}

void stage5_draw(void) {
	stage3d_draw(&stage_3d_context, 50, 1, (Stage3DSegment[]) { stage5_stairs_draw, stage5_stairs_pos });
}

static bool stage5_fog(Framebuffer *fb) {
	r_shader("zbuf_fog");
	r_uniform_sampler("depth", r_framebuffer_get_attachment(fb, FRAMEBUFFER_ATTACH_DEPTH));
	r_uniform_vec4_rgba("fog_color", color_mul_scalar(RGB(0.3,0.1,0.8), 0.25));
	r_uniform_float("start", 0.2);
	r_uniform_float("end", 3.8);
	r_uniform_float("exponent", 3.0);
	r_uniform_float("sphereness", 0);
	draw_framebuffer_tex(fb, VIEWPORT_W, VIEWPORT_H);
	r_shader_standard();
	return true;
}

void iku_spell_bg(Boss *b, int t) {
	fill_viewport(0, 300, 1, "stage5/spell_bg");

	r_blend(BLEND_MOD);
	fill_viewport(0, t*0.001, 0.7, "stage5/noise");
	r_blend(BLEND_PREMUL_ALPHA);

	r_mat_mv_push();
	r_mat_mv_translate(0, -100, 0);
	fill_viewport(t/100.0, 0, 0.5, "stage5/spell_clouds");
	r_mat_mv_translate(0, 100, 0);
	fill_viewport(t/100.0 * 0.75, 0, 0.6, "stage5/spell_clouds");
	r_mat_mv_translate(0, 100, 0);
	fill_viewport(t/100.0 * 0.5, 0, 0.7, "stage5/spell_clouds");
	r_mat_mv_translate(0, 100, 0);
	fill_viewport(t/100.0 * 0.25, 0, 0.8, "stage5/spell_clouds");
	r_mat_mv_pop();

	float opacity = 0.05 * stage5_draw_data->stairs.light_strength;
	r_color4(opacity, opacity, opacity, opacity);
	fill_viewport(0, 300, 1, "stage5/spell_lightning");
}

ShaderRule stage5_bg_effects[] = {
	stage5_fog,
	NULL
};
