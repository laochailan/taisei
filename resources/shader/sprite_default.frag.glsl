#version 330 core

#include "interface/sprite.glslh"

void main(void) {
    fragColor = color * texture(tex, texCoord);
}
