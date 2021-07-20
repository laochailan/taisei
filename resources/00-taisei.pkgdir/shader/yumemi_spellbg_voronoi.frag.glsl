#version 330 core

#include "interface/standard.glslh"
#include "lib/util.glslh"

/*
 * Based on https://www.shadertoy.com/view/MtlyR8
 */

#define rnd(p)   fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453)
#define srnd(p)  (2.0 * rnd(p) - 1.0)

#define N 3

UNIFORM(1) float time;
UNIFORM(2) vec4 color;

void main(void) {
	vec2 uv = 12.0 * texCoordRaw;
	float m = 1e9, m2, c = 1e2, v, w;

	// visit 3x3 neighbor tiles
	for(int k = 0; k < N * N; ++k) {
		// tile center
		vec2 iU = floor(uv) + 0.5;

		// neighbor cell
		vec2 g = iU + vec2(k % N, k / N) - 1.0;

		float jitter = srnd(g);

		// vector to jittered cell node
		vec2 p = g + 0.1 * jitter - uv;
		p += 0.1 * sin(time + vec2(1.6, 0.0) + pi * jitter);

		// --- choose distance kind ------------------

		// L2 distance
		// v = length(p);

		// L1 distance
		v = abs(p.x) + abs(p.y);

		// Linfinity distance ( = Manhattan = taxicab distance)
		// p = abs(p); v = max(p.x, p.y);

		if(v < m) {
			// keep 1st and 2nd min distances to node
			m2 = m;
			m = v;
		} else if(v < m2) {
			m2 = v;
		}
    }

	float f = 0.5 + 0.5 * sin(time - uv.y + 2.0 * m);
	f = smoothstep(m2 - m, m2 * m, f);
	v = mix(m2 * m, m2 - m, f);

	fragColor = color / v;
}
