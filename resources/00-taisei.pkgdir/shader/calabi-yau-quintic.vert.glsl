#version 330

#include "lib/util.glslh"
#include "lib/render_context.glslh"
#include "interface/standard.glslh"

UNIFORM(1) float alpha;
VARYING(4) vec3 pos;

vec2 cexp(vec2 z) {
	return dir(z.y) * exp(z.x);
}

vec2 clog(vec2 z) {
	return vec2(log(length(z)), angle(z));
}

vec2 csin(vec2 z) {
	return cmul(vec2(0,-0.5), cexp(rot(pi/2)*z) - cexp(-rot(pi/2)*z));
}

vec2 ccos(vec2 z) {
	return 0.5*(cexp(rot(pi/2)*z) + cexp(-rot(pi/2)*z));
}

vec2 cpow(vec2 z, float b) {
	if(length(z) < 0.01) {
		return vec2(0,0);
	} else {
		return cexp(clog(z)*b);
	}
}

// adjusted from https://tex.stackexchange.com/questions/143123/how-to-draw-the-logo-of-the-string-theory

void main(void) {
	vec2 z = vec2(position.x, pi/4*(position.y+1.00001)/1.00001);
	float n = 5;
	float k1 = floor(position.z/n);
	float k2 = mod(position.z,n);

	vec2 z1, z2;

	z1 = rot(tau*k1/n)*cpow(ccos(rot(pi/2)*z),2/n);
	z2 = rot(tau*k2/n)*cpow(-rot(pi/2)*csin(rot(pi/2)*z),2/n);

	vec3 zpos = vec3(z2.x, cos(alpha)*z1.y + sin(alpha)*z2.y, z1.x);


	gl_Position = r_projectionMatrix * r_modelViewMatrix * vec4(zpos,1);

	pos = zpos; //(r_modelViewMatrix * vec4(zpos, 1)).xyz;

	texCoord = (r_textureMatrix * vec4(texCoordRawIn, 0.0, 1.0)).xy;
	texCoordRaw = texCoordRawIn;
}
