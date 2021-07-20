#version 330 core

#include "lib/render_context.glslh"
#include "interface/standard.glslh"
#include "lib/util.glslh"

VARYING(4) vec3 pos;
void main(void) {
	fragColor = vec4(0.5*normalize(pos)+vec3(0.5), 1);
}
