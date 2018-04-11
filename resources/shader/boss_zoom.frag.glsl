#version 330 core

#include "lib/render_context.glslh"
#include "interface/standard.glslh"

UNIFORM(1) vec2 blur_orig; // center
UNIFORM(2) vec2 fix_orig;
UNIFORM(3) float blur_rad;  // radius of zoom effect
UNIFORM(4) float rad;
UNIFORM(5) float ratio; // texture h/w
UNIFORM(6) vec4 color;

void main(void) {
	vec2 pos = texCoordRaw;
	pos -= blur_orig;

	vec2 pos1 = pos;
	pos1.y *= ratio;

	pos *= min(length(pos1)/blur_rad,1.0);
	pos += blur_orig;
	pos = clamp(pos, 0.005, 0.995);
	pos = (r_textureMatrix * vec4(pos,0.0,1.0)).xy;

	fragColor = texture(tex, pos);

	pos1 = texCoordRaw - fix_orig;
	pos1.y *= ratio;

	fragColor *= pow(color,vec4(3.0*max(0.0,rad - length(pos1))));
}
