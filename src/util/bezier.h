/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#ifndef IGUARD_util_bezier_h
#define IGUARD_util_bezier_h

#include "taisei.h"

typedef union CubicBezier {
	struct {
		cmplxf start, control[2], end;
	};
	struct {
		cmplxf points[4];
	};
} CubicBezier;

cmplxf cbezier_point_at(CubicBezier curve, float t) attr_const;
cmplxf cbezier_derivative_at(CubicBezier curve, float t) attr_const;
cmplxf cbezier_derivative2_at(CubicBezier curve, float t) attr_const;

void cbezier_scale(CubicBezier *curve, cmplxf x);
void cbezier_translate(CubicBezier *curve, cmplxf x);
void cbezier_rotate(CubicBezier *curve, cmplxf pivot, float angle);

#endif // IGUARD_util_bezier_h
