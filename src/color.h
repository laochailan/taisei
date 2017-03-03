/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#ifndef TSCOLOR_H
#define TSCOLOR_H

#include <stdint.h>

#define CLR_R 48L
#define CLR_G 32L
#define CLR_B 16L
#define CLR_A 0L

#define CLR_CMASK 0xffffL
#define CLR_ONEVALUE 0xffL

#define CLRMASK_R (CLR_CMASK << CLR_R)
#define CLRMASK_G (CLR_CMASK << CLR_G)
#define CLRMASK_B (CLR_CMASK << CLR_B)
#define CLRMASK_A (CLR_CMASK << CLR_A)

typedef uint64_t Color;
typedef int16_t ColorComponent;

Color rgba(float r, float g, float b, float a);
Color rgb(float r, float g, float b);
void parse_color(Color clr, float *r, float *g, float *b, float *a);
void parse_color_call(Color clr, void(*func)(float r, float g, float b, float a));
void parse_color_array(Color clr, float array[4]);
Color derive_color(Color src, Color mask, Color mod);
Color multiply_colors(Color c1, Color c2);
Color add_colors(Color c1, Color c2);
Color subtract_colors(Color c1, Color c2);
float color_component(Color clr, unsigned int ofs);

int color_test(void);

#endif
