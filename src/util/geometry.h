/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_util_geometry_h
#define IGUARD_util_geometry_h

#include "taisei.h"

typedef union FloatOffset {
	struct { float x, y; };
	cmplx32 as_cmplx;
} FloatOffset;

typedef union FloatExtent {
	struct { float w, h; };
	cmplx32 as_cmplx;
} FloatExtent;

typedef struct FloatRect {
	union {
		FloatOffset offset;
		struct { float x, y; };
	};
	union {
		FloatExtent extent;
		struct { float w, h; };
	};
} FloatRect;

typedef struct IntOffset {
	int x, y;
} IntOffset;

typedef struct IntExtent {
	int w, h;
} IntExtent;

typedef struct IntRect {
	union {
		IntOffset offset;
		struct { int x, y; };
	};
	union {
		IntExtent extent;
		struct { int w, h; };
	};
} IntRect;

typedef struct Ellipse {
	cmplx origin;
	cmplx axes; // NOT half-axes!
	double angle;
} Ellipse;

typedef struct LineSegment {
	cmplx a;
	cmplx b;
} LineSegment;

typedef struct Circle {
	cmplx origin;
	double radius;
} Circle;

typedef struct Rect {
	cmplx top_left;
	cmplx bottom_right;
} Rect;

bool point_in_ellipse(cmplx p, Ellipse e) attr_const;
double lineseg_circle_intersect(LineSegment seg, Circle c) attr_const;
bool lineseg_ellipse_intersect(LineSegment seg, Ellipse e) attr_const;

INLINE attr_const
double rect_x(Rect r) {
	return creal(r.top_left);
}

INLINE attr_const
double rect_y(Rect r) {
	return cimag(r.top_left);
}

INLINE attr_const
double rect_width(Rect r) {
	return creal(r.bottom_right) - creal(r.top_left);
}

INLINE attr_const
double rect_height(Rect r) {
	return cimag(r.bottom_right) - cimag(r.top_left);
}

INLINE attr_const
double rect_top(Rect r) {
	return cimag(r.top_left);
}

INLINE attr_const
double rect_bottom(Rect r) {
	return cimag(r.bottom_right);
}

INLINE attr_const
double rect_left(Rect r) {
	return creal(r.top_left);
}

INLINE attr_const
double rect_right(Rect r) {
	return creal(r.bottom_right);
}

INLINE attr_const
double rect_area(Rect r) {
	return rect_width(r) * rect_height(r);
}

INLINE
void rect_move(Rect *r, cmplx pos) {
	cmplx vector = pos - r->top_left;
	r->top_left += vector;
	r->bottom_right += vector;
}

bool point_in_rect(cmplx p, Rect r);
bool rect_in_rect(Rect inner, Rect outer) attr_const;
bool rect_rect_intersect(Rect r1, Rect r2, bool edges, bool corners) attr_const;
bool rect_rect_intersection(Rect r1, Rect r2, bool edges, bool corners, Rect *out) attr_pure attr_nonnull(5);
bool rect_join(Rect *r1, Rect r2) attr_pure attr_nonnull(1);
void rect_set_xywh(Rect *rect, double x, double y, double w, double h) attr_nonnull(1);

#endif // IGUARD_util_geometry_h
