/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#include <stdio.h>
#include <stdlib.h>
#include "random.h"
#include "taisei_err.h"

/*
 *	Multiply-with-carry algorithm
 */

static RandomState *tsrand_current;

void tsrand_switch(RandomState *rnd) {
	tsrand_current = rnd;
}

void tsrand_seed_p(RandomState *rnd, uint32_t seed) {
	// might be not the best way to seed this
	rnd->w = (seed >> 16u) + TSRAND_W_SEED_COEFF * (seed & 0xFFFFu);
	rnd->z = (seed >> 16u) + TSRAND_Z_SEED_COEFF * (seed & 0xFFFFu);
}

int tsrand_p(RandomState *rnd) {
	rnd->w = (rnd->w >> 16u) + TSRAND_W_COEFF * (rnd->w & 0xFFFFu);
	rnd->z = (rnd->z >> 16u) + TSRAND_Z_COEFF * (rnd->z & 0xFFFFu);
	return (uint32_t)((rnd->z << 16u) + rnd->w) % TSRAND_MAX;
}

inline void tsrand_seed(uint32_t seed) {
	tsrand_seed_p(tsrand_current, seed);
}

inline int tsrand(void) {
	return tsrand_p(tsrand_current);
}

inline double frand(void) {
	return tsrand()/(double)TSRAND_MAX;
}

inline double nfrand() {
	return frand() * 2 - 1;
}

int tsrand_test(void) {
#if defined(TSRAND_FLOATTEST)
	RandomState rnd;
	tsrand_switch(&rnd);
	tsrand_seed(time(0));
	
	FILE *fp;
	int i;
	
	fp = fopen("/tmp/rand_test", "w");
	for(i = 0; i < 10000; ++i)
		fprintf(fp, "%f\n", frand());
	
	return 1;
#elif defined(TSRAND_SEEDTEST)
	RandomState rnd;
	tsrand_switch(&rnd);
	
	int seed = 1337, i, j;
	printf("SEED: %d\n", seed);
	
	for(i = 0; i < 5; ++i) {
		printf("RUN #%i\n", i);
		tsrand_seed(seed);
		
		for(j = 0; j < 5; ++j) {
			printf("-> %i\n", tsrand());
		}
	}
	
	return 1;
#else
	return 0;
#endif
}

// we use this to support multiple rands in a single statement without breaking replays across different builds

static int tsrand_array[TSRAND_ARRAY_LIMIT];
static int tsrand_array_elems;

void tsrand_fill_p(RandomState *rnd, int amount) {
	tsrand_array_elems = amount;
	int i; for(i = 0; i < amount; ++i)
		tsrand_array[i] = tsrand_p(rnd);
}

inline void tsrand_fill(int amount) {
	tsrand_fill_p(tsrand_current, amount);
}

inline int tsrand_a(int idx) {
	if(idx >= tsrand_array_elems || idx < 0)
		errx(-1, "tsrand_a: index out of range (%i / %i)", idx, tsrand_array_elems);
	return tsrand_array[idx];
}

inline double afrand(int idx) {
	return tsrand_a(idx)/(double)TSRAND_MAX;
}

inline double anfrand(int idx) {
	return afrand(idx) * 2 - 1;
}

