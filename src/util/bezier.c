/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "bezier.h"
#include "miscmath.h"

cmplxf cbezier_point_at(CubicBezier curve, float t) {
	float u = 1.0f - t;
	float uu = u * u;
	float tt = t * t;
	float uuu = uu * u;
	float uut3 = uu * t * 3.0f;
	float utt3 = u * tt * 3.0f;
	float ttt = tt * t;

	return
		curve.points[0] * uuu +
		curve.points[1] * uut3 +
		curve.points[2] * utt3 +
		curve.points[3] * ttt;
}

cmplxf cbezier_derivative_at(CubicBezier curve, float t) {
	float u = 1.0f - t;

	return
		(curve.points[1] - curve.points[0]) * (3.0f * u * u) +
		(curve.points[2] - curve.points[1]) * (6.0f * u * t) +
		(curve.points[3] - curve.points[2]) * (3.0f * t * t);
}

cmplxf cbezier_derivative2_at(CubicBezier curve, float t) {
	float t6 = 6.0f * t;
	float u6 = 6.0f - t6;

	return
		(curve.points[2] - 2.0f * curve.points[1] + curve.points[0]) * u6 +
		(curve.points[3] - 2.0f * curve.points[2] + curve.points[1]) * t6;
}

void cbezier_scale(CubicBezier *curve, cmplxf x) {
	curve->points[0] *= x;
	curve->points[1] *= x;
	curve->points[2] *= x;
	curve->points[3] *= x;
}

void cbezier_translate(CubicBezier *curve, cmplxf x) {
	curve->points[0] += x;
	curve->points[1] += x;
	curve->points[2] += x;
	curve->points[3] += x;
}

void cbezier_rotate(CubicBezier *curve, cmplxf pivot, float angle) {
	cbezier_translate(curve, -pivot);
	cbezier_scale(curve, cdirf(angle));
	cbezier_translate(curve, pivot);
}
