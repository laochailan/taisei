#version 330 core

#include "lib/render_context.glslh"
#include "interface/standard.glslh"

UNIFORM(1) sampler2D tex2;

void main(void) {
	fragColor = vec4(texture(tex, texCoord).rgb * texture(tex2, texCoordRaw).rgb, 1.0);
}
