#version 330 core

#include "lib/render_context.glslh"
#include "interface/standard.glslh"

// UNIFORM(1) vec3 time_offsets = vec3(0.143133, 0.53434, 0.25332);
// UNIFORM(2) vec3 time_scales = vec3(1.0);

UNIFORM(1) vec3 times;
UNIFORM(2) vec3 cutoffs = vec3(0.53, 0.56, 0.51);
UNIFORM(3) vec2 grid = vec2(8.0, 16.0);
UNIFORM(4) vec3 waverectfactors = vec3(7.0);
UNIFORM(5) sampler2D mask;

float hash(vec2 co) {
	float a = 12.9898;
	float b = 78.233;
	float c = 43758.5453;
	float dt = dot(co, vec2(a, b));
	float sn = mod(dt, 3.14);
	return fract(sin(sn) * c);
}

float wave(float x) {
	float y = sin(x) + cos(3.0*x) + sin(5.0*x) + cos(7.0*x);
	float m = 3.0;
	return clamp((m + y) / (2.0 * m), 0.0, 1.0);
}

vec3 wave(vec3 v) {
	return vec3(
		wave(v.x),
		wave(v.y),
		wave(v.z)
    );
}

float rect(float x, float g) {
	return round(g * x) / g;
}

vec2 rect(vec2 v, vec2 g) {
	return vec2(
		rect(v.x, g.x),
		rect(v.y, g.y)
	);
}

vec3 rect(vec3 v, vec3 g) {
	return vec3(
		rect(v.x, g.x),
		rect(v.y, g.y),
		rect(v.z, g.z)
	);
}

vec3 glitch(vec2 uv, vec3 time, vec2 grid, vec3 cutoffs, vec3 waverectfactors) {
	uv = rect(uv, grid);
	vec3 t = time;
	vec3 v = wave(t);
	vec3 mstatic = step(cutoffs, v);
	vec3 mchaotic = step(cutoffs, v * v);
    v = rect(v, waverectfactors);
	vec3 mask = vec3(
		hash(uv + t.r * mchaotic.r),
		hash(uv + t.g * mchaotic.g),
		hash(uv + t.b * mchaotic.b)
	);
	mask = step(mstatic * v, mask);
	mask = (1.0 - mask);
	// mask *= v;
	return mask;
}
#if 0
void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = fragCoord/iResolution.xy;

    vec2 grid = vec2(8.0, 16.0);
    vec3 cutoffs = vec3(0.53, 0.56, 0.51) * 2.0 * iMouse.x/iResolution.x;
    vec3 timeofs = vec3(0.143133, 0.53434, 0.25332) * 0.1;
    vec3 waverectfactors = vec3(7.0);
    vec3 k = glitch(uv, iTime, grid, cutoffs, timeofs, waverectfactors);

    vec4 tex = texture(iChannel1, uv);
    vec4 tex_ofs = vec4(1);// = texture(iChannel1, uv - 1.0 / grid);

    vec3 o = sin(3.14*2.0*k);

    tex_ofs.r = texture(iChannel1, uv - o.r / grid).r;
    tex_ofs.g = texture(iChannel1, uv - o.g / grid).g;
    tex_ofs.b = texture(iChannel1, uv - o.b / grid).b;

    tex.rgb = mix(tex.rgb, tex_ofs.rgb, k);
    // tex.rgb = k;


    // tex.r = A;

    fragColor = tex;
}
#endif
void main(void) {
	vec2 uv = texCoord;

	vec4 texNormal = texture(tex, uv);
	vec3 m = texture(mask, uv).rgb;
	float a = m.r + m.g + m.b;

	if(a == 0) {
		fragColor = texNormal;
		return;
	} else {
		// fragColor = vec4(a,0,0,1);
		// return;
	}

	// vec3 t = float(frames) / 60.0 * time_scales + time_offsets;
	vec3 k = glitch(uv, times, grid, cutoffs, waverectfactors);
	vec3 o = k * a * 0.25;

	vec3 texGlitched;
	texGlitched.r = texture(tex, uv - o.r / grid).r;
	texGlitched.g = texture(tex, uv - o.g / grid).g;
	texGlitched.b = texture(tex, uv - o.b / grid).b;

	fragColor.rgb = mix(texNormal.rgb, texGlitched, k);
	fragColor.a = texNormal.a;
}
