#version 330 core

#include "interface/sprite.glslh"
#include "lib/util.glslh"
#include "lib/render_context.glslh"

void main(void) {
    gl_Position = r_projectionMatrix * spriteVMTransform * vec4(vertPos, 0.0, 1.0);

    color           = spriteRGBA;
    texCoordRaw     = vertTexCoord;
    texCoord        = uv_to_region(spriteTexRegion, vertTexCoord);
    // texCoordOverlay = (spriteTexTransform * vec4(vertTexCoord, 0.0, 1.0)).xy;
    // texRegion       = spriteTexRegion;
    // dimensions      = spriteDimensions;
    customParams    = spriteCustomParams;
}
