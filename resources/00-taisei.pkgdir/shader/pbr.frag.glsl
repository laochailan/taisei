#version 330 core

#include "lib/render_context.glslh"
#include "lib/parallaxmap.glslh"
#include "interface/pbr.glslh"

void main(void) {
	float roughness = ambientRGB_roughnessA.a;
	float alpha;

	mat3 tbn = mat3(normalize(tangent), normalize(bitangent), normalize(normal));
	vec2 uv = texCoord;

	if(bool(features_mask & PBR_FEATURE_DEPTH_MAP)) {
		vec3 vdir = normalize(transpose(tbn) * -pos);
		uv = parallaxOcclusionMap(depth_map, depth_scale, uv, vdir);

		/*
		if(uv.x > 1.0 || uv.y > 1.0 || uv.x < 0.0 || uv.y < 0.0) {
			discard;
		}
		*/
	}

	if(bool(features_mask & PBR_FEATURE_ROUGHNESS_MAP)) {
		vec4 roughness_sample = texture(roughness_map, uv);
		roughness *= roughness_sample.r;
		alpha = roughness_sample.a;

		// TODO: a way to opt out of this, since it hurts performance.
		if(alpha < 0.3) {
			discard;
		}
	} else {
		alpha = 1.0;
	}

	float metallic = diffuseRGB_metallicA.a;
	vec3 diffuse = diffuseRGB_metallicA.rgb;

	if(bool(features_mask & PBR_FEATURE_DIFFUSE_MAP)) {
		diffuse *= texture(diffuse_map, uv).rgb;
	}

	vec3 ambient = ambientRGB_roughnessA.rgb;

	if(bool(features_mask & PBR_FEATURE_AMBIENT_MAP)) {
		ambient *= texture(ambient_map, uv).rgb;
	}

	vec3 tbn_normal;

	if(bool(features_mask & PBR_FEATURE_NORMAL_MAP)) {
		tbn_normal = sample_normalmap(normal_map, uv);
	} else {
		tbn_normal = vec3(0, 0, 1);
	}

	PBRParams p;
	p.fragPos = pos;
	p.albedo = diffuse;
	p.roughness = roughness;
	p.metallic = metallic;
	p.normal = tbn * tbn_normal;
	p.inv_camera_transform = inv_camera_transform;

	PBRState pbr = PBR(p);

	vec3 color = ambient;

	for(int i = 0; i < light_count; ++i) {
		color += PBR_PointLight(pbr, PointLight(light_positions[i], light_colors[i]));
	}

	if(bool(features_mask & PBR_FEATURE_ENVIRONMENT_MAP)) {
		color += PBR_EnvironmentLight(pbr, ibl_brdf_lut, environment_map);
	}

	color = PBR_TonemapReinhard(color);
	color = PBR_GammaCorrect(color);

	fragColor = vec4(color * alpha, alpha);
}
