#version 330 core

#include "interface/sprite.glslh"
#include "lib/util.glslh"
#include "lib/sprite_main.frag.glslh"
#include "extra_bg.glslh"

void spriteMain(out vec4 fragColor) {
	float time = customParams.x;
	vec2 code_aspect = customParams.yz;
	float num_segs = customParams.w;
	float inv_num_segs = 1.0 / num_segs;

	vec4 mask = texture(tex, texCoord);
	vec2 codeUV = texCoordRaw + mask.rb * (1 - mask.g) * 0.1;

	vec4 code_params = vec4(code_aspect * vec2(1.5, 1), num_segs, inv_num_segs);
	vec4 code = sample_code(tex_aux[0], code_params, codeUV + vec2(0.03, -0.05) * time);

	float m = 0.5 + 0.5 * sin(time * 3.21 + codeUV.y * 96.31);
	float ca = (1 - smoothstep(0.01, 0.2, mix(mask.b, mask.r, m)));
	ca = sin(4 * pi * ca + 32.311 * codeUV.x - 1.31 * time);
	ca = smoothstep(-0.5, 1.0, ca);

	code.rgb = 1 - pow(mask.rgb, code.rgb);
	code.g *= 1 - 2 * (mask.b + mask.r);
	code.rgb *= ca * 0.5;

	fragColor = vec4(code.rgb + mask.rgb * code.a * 2, 0);

	code = sample_code(tex_aux[0], code_params, codeUV + vec2(-0.0359, -0.04837) * time);
	fragColor.rgb += vec3(mask.rgb * code.a * 2);

	fragColor *= color * mask.a;

//	fragColor = color * vec4(mask.ggg, mask.a);

	// fragColor = vec4(vec3(ca, code.a, 0), 1);
	// fragColor = smoothstep(0.05, 0.1, mask);
//	fragColor = vec4(codeUV.x, codeUV.y, 0, 1.0);
}
