#version 330 core

uniform sampler2D tex;

in vec3 posModelView;
in vec2 texCoord;
out vec4 fragColor;

void main(void) {
	vec4 texel = texture(tex, texCoord);
	float f = min(1.0, length(posModelView)/3000.0);
	fragColor = mix(vec4(1.0), texel, 1.0-f);
}
