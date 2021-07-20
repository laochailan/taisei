#version 330 core

#include "interface/standard.glslh"
#include "lib/util.glslh"

VARYING(3) vec3 posRaw;
UNIFORM(1) samplerCube skybox;

void main(void) {
	fragColor = texture(skybox, fixCubeCoord(posRaw));

	fragColor.r = linear_to_srgb(fragColor.r);
	fragColor.g = linear_to_srgb(fragColor.g);
	fragColor.b = linear_to_srgb(fragColor.b);
}
