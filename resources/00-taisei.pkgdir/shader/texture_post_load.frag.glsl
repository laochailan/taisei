#version 330 core

#include "interface/standard.glslh"
#include "lib/util.glslh"

UNIFORM(1) sampler2D alphamap;

UNIFORM(2) int linearize;
UNIFORM(3) int multiply_alpha;
UNIFORM(4) int apply_alphamap;
UNIFORM(5) int write_srgb;

void main(void) {
	fragColor = textureLod(tex, texCoordRaw, 0);

	if(linearize != 0) {
		fragColor.rgb = vec3(
			srgb_to_linear(fragColor.r),
			srgb_to_linear(fragColor.g),
			srgb_to_linear(fragColor.b)
		);
	}

	if(multiply_alpha != 0) {
		fragColor.rgb *= fragColor.a;
	}

	if(apply_alphamap != 0) {
		fragColor.a *= textureLod(alphamap, texCoordRaw, 0).r;
	}

	if(write_srgb != 0) {
		fragColor.rgb = vec3(
			linear_to_srgb(fragColor.r),
			linear_to_srgb(fragColor.g),
			linear_to_srgb(fragColor.b)
		);
	}
}
