/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_stageutils_h
#define IGUARD_stageutils_h

#include "taisei.h"

#include "util.h"
#include "resource/material.h"
#include "global.h" // remove when STAGE3D_DEFAULT_ASPECT aspect is removed

typedef struct Stage3D Stage3D;

typedef void (*SegmentDrawRule)(vec3 pos);
typedef uint (*SegmentPositionRule)(Stage3D *s3d, vec3 q, float maxrange); // returns number of elements written to Stage3D pos_buffer

typedef struct Stage3DSegment {
	SegmentDrawRule draw;
	SegmentPositionRule pos;
} Stage3DSegment;

typedef union Camera3DRotation {
	struct { float pitch, yaw, roll; };
	vec3 v;
} Camera3DRotation;

typedef struct Camera3D {
	vec3 pos;
	vec3 vel;
	Camera3DRotation rot;

	real fovy;
	real aspect;
	real near;
	real far;

} Camera3D;

typedef struct PointLight3D {
	vec3 pos;
	vec3 radiance;
} PointLight3D;

// NOTE: should match PBR_MAX_LIGHTS in lib/pbr.glslh
#define STAGE3D_MAX_LIGHTS 6

// NOTE: should match definitions in interface/pbr.glslh
#define PBR_FEATURE_DIFFUSE_MAP         1
#define PBR_FEATURE_NORMAL_MAP          2
#define PBR_FEATURE_AMBIENT_MAP         4
#define PBR_FEATURE_ROUGHNESS_MAP       8
#define PBR_FEATURE_ENVIRONMENT_MAP    16
#define PBR_FEATURE_DEPTH_MAP          32

typedef struct PBREnvironment {
	mat4 cam_inverse_transform;
	Texture *environment_map;
	vec3 ambient_color;
} PBREnvironment;

typedef struct PBRModel {
	Model *mdl;
	PBRMaterial *mat;
} PBRModel;

#define STAGE3D_DEPRECATED(...) attr_deprecated(__VA_ARGS__)

struct Stage3D {
	union {
		Camera3D cam;

		struct {
			vec3 cx     STAGE3D_DEPRECATED("Use .cam.pos instead");
			vec3 cv     STAGE3D_DEPRECATED("Use .cam.vel instead");
			vec3 crot   STAGE3D_DEPRECATED("Use .cam.rot instead");
		};
	};

	vec3 *pos_buffer;
	uint pos_buffer_size;
};

extern Stage3D stage_3d_context;

#define STAGE3D_DEFAULT_FOVY ((float)M_PI / 4.258f)
#define STAGE3D_DEFAULT_ASPECT ((0.75f * VIEWPORT_W) / VIEWPORT_H) // deprecated
#define STAGE3D_DEFAULT_NEAR 1
#define STAGE3D_DEFAULT_FAR 60

void stage3d_init(Stage3D *s, uint pos_buffer_size);
void stage3d_update(Stage3D *s);
void stage3d_shutdown(Stage3D *s);
void stage3d_apply_transforms(Stage3D *s, mat4 mat);
void stage3d_apply_inverse_transforms(Stage3D *s, mat4 mat);
void stage3d_draw_segment(Stage3D *s, SegmentPositionRule pos_rule, SegmentDrawRule draw_rule, float maxrange);
void stage3d_draw(Stage3D *s, float maxrange, uint nsegments, const Stage3DSegment segments[nsegments]);

void camera3d_init(Camera3D *cam) attr_nonnull(1);
void camera3d_update(Camera3D *cam) attr_nonnull(1);
void camera3d_apply_transforms(Camera3D *cam, mat4 mat) attr_nonnull(1, 2);
void camera3d_apply_inverse_transforms(Camera3D *cam, mat4 mat) attr_nonnull(1, 2);
void camera3d_unprojected_ray(Camera3D *cam, cmplx pos, vec3 dest) attr_nonnull(1, 3);

void camera3d_set_point_light_uniforms(
	Camera3D *cam,
	uint num_lights,
	PointLight3D lights[num_lights]
) attr_nonnull(1, 3);

void camera3d_fill_point_light_uniform_vectors(
	Camera3D *cam,
	uint num_lights,
	PointLight3D lights[num_lights],
	vec3 out_lpos[num_lights],
	vec3 out_lrad[num_lights]
) attr_nonnull(1, 3, 4);

void pbr_set_material_uniforms(const PBRMaterial *m, const PBREnvironment *env) attr_nonnull_all;
void pbr_draw_model(const PBRModel *pmdl, const PBREnvironment *env) attr_nonnull_all;
void pbr_load_model(PBRModel *pmdl, const char *model_name, const char *mat_name);

uint linear3dpos(Stage3D *s3d, vec3 q, float maxrange, vec3 p, vec3 r);
uint single3dpos(Stage3D *s3d, vec3 q, float maxrange, vec3 p);

void skip_background_anim(void (*update_func)(void), int frames, int *timer, int *timer2);

#endif // IGUARD_stageutils_h
