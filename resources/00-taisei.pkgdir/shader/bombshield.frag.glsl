#version 330 core

#include "lib/util.glslh"
#include "interface/standard.glslh"

UNIFORM(1) sampler2D shell_tex;
UNIFORM(2) sampler2D grid_tex;
UNIFORM(3) vec2 grid_aspect;
UNIFORM(4) sampler2D code_tex;
UNIFORM(5) vec2 code_aspect;

UNIFORM(6) float time;
UNIFORM(7) float alpha;

vec2 edts(vec2 p) {
	vec2 uv = p * p;
	vec2 s = 2.0 + uv - uv.yx;
	vec2 t = (2.0 * sqrt(2.0)) * p;
	vec2 t1 = s + t;
	vec2 t2 = s - t;
	// t1 = abs(t1); t2 = abs(t2);
	return 0.5 * (sqrt(t1) - sqrt(t2));
}

void main(void) {
	vec2 uv = texCoord;
	float f = distance(uv, vec2(0.5));

	if(f > 0.5) {
		discard;
	}

	f = 1.0 - 2.0 * f;
	float t = time;
	vec2 warped_uv = edts((2.0 - pow(f, 1.0/3.0)) * (uv - 0.5)) + 0.5;

	vec2 grid = texture(grid_tex, grid_aspect * (warped_uv + vec2(t * 2.0, 0.0))).rg;

	vec3 c0 = vec3(0.85 * (warped_uv.y), 0.5, 1.0 - 0.5 * warped_uv.y);
	c0 = pow(texture(shell_tex, warped_uv + vec2(t * 1.3, t * 0.73)).rgb, 1.0 - c0);

	vec3 c1 = texture(code_tex, code_aspect * (warped_uv + vec2(t * 2.1, t * -0.42))).rrr;
	c1 *= vec3(0.2, 0.7, 1.0);

	float ring = clamp(smoothstep(0.0, 0.03, f) * (1.0 - f), 0.0, 1.0);
	float bg = smoothstep(0.0, 0.1, f);
	float h = smoothmax(bg * grid.r, ring, grid.r * (10 - 8 * ((1 - f * f) * alpha)) + ring);

	float alpha2 = alpha * alpha;

	c0 *= h;
	c1 *= grid.g * alpha2;

	fragColor = vec4((c0 + c1) * alpha2, 0.6 * bg * alpha);
}
