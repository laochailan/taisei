/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "../stagex.h"
#include "global.h"
#include "util/glm.h"
#include "common_tasks.h"

// TODO naming not ideal… CELLS vs CELL
#define CELLS_W 3
#define CELLS_H 3
#define CELL_W 6
#define CELL_H 8
#define CLEAR_TIME 240
#define CELL_SIZE CMPLX(VIEWPORT_W/CELLS_W, VIEWPORT_H/CELLS_H)

TASK(cell_proj_aim, { BoxedProjectile pbox; }) {
	play_sfx("redirect");
	Projectile *p = TASK_BIND(ARGS.pbox);

	cmplx aim = cnormalize(global.plr.pos-p->pos);
	p->move = move_accelerated(0,0.01*aim);
	projectile_set_prototype(p, pp_ball);
	p->color = *RGBA(0.2, 0, 1, 1);
}

typedef enum {
	CD_NONE,
	CD_UP,
	CD_RIGHT,
	CD_DOWN,
	CD_LEFT,
	// CD_PROJEKT_RED,
	CD_FIRST = CD_UP,
	CD_LAST = CD_LEFT,
} CellDirection;


static void cell_split_idx(int cell, int *x, int *y) {
	*x = cell % CELLS_W;
	*y = cell / CELLS_W;
}

static int find_player_cell(void) {
	int x = creal(global.plr.pos) / VIEWPORT_W * CELLS_W;
	int y = cimag(global.plr.pos) / VIEWPORT_H * CELLS_H;

	return y * CELLS_W + x;
}

static void cell_apply_dir_xy(int x, int y, CellDirection dir, int *nx, int *ny) {
	*nx = x;
	*ny = y;
	switch(dir) {
	case CD_UP:
		(*ny)--;
		break;
	case CD_DOWN:
		(*ny)++;
		break;
	case CD_RIGHT:
		(*nx)++;
		break;
	case CD_LEFT:
		(*nx)--;
		break;
	default:
		break;
	}
}

static int cell_apply_dir(int cell, CellDirection dir) {
	int x, y;
	cell_split_idx(cell, &x, &y);

	int nx =-1, ny=-1;
	cell_apply_dir_xy(x, y, dir, &nx, &ny);

	assert(nx >= 0 && nx < CELLS_W);
	assert(ny >= 0 && ny < CELLS_H);

	return ny*CELLS_W + nx;
}

static CellDirection cell_find_expansion_dir(int cell) {
	int x, y;
	cell_split_idx(cell, &x, &y);

	cmplx dir;
	int nx = -1, ny = -1;
	do {
		dir = rng_irange(CD_FIRST,CD_LAST+1);
		cell_apply_dir_xy(x, y, dir, &nx, &ny);
	} while(nx < 0 || ny < 0 || nx >= CELLS_W || ny >= CELLS_H);

	return dir;
}

static real cell_dir_transition(int cell, CellDirection dir) {
	int x = cell % CELL_W;
	int y = cell / CELL_W;
	switch(dir) {
	case CD_UP:
		return (CELL_H-y)/(real)CELL_H;
	case CD_DOWN:
		return y/(real)CELL_H;
	case CD_LEFT:
		return (CELL_W-x)/(real)CELL_W;
	case CD_RIGHT:
		return x/(real)CELL_W;
	default:
		break;
	}

	return 0;
}

static cmplx cell_topleft(int cell) {
	int cx, cy;
	cell_split_idx(cell, &cx, &cy);

	return CMPLX(cx*creal(CELL_SIZE), cy*cimag(CELL_SIZE));
}

TASK(spawn_cell, { int idx; int missing; CoEvent *destroy; CellDirection *clear_dir; }) {
	play_sfx("shot2");
	BoxedProjectile projs[CELL_W*CELL_H];
	memset(projs, 0, sizeof(projs));

	cmplx topleft = cell_topleft(ARGS.idx);

	assert(ARGS.missing >= 0);

	for(int y = 0; y < CELL_H; y++) {
		for(int x = 0; x < CELL_W; x++) {
			if(y*CELL_W+x == ARGS.missing) {
				continue;
			}
			cmplx offset = CMPLX(creal(CELL_SIZE)*(x+0.5)/CELL_W,
					     cimag(CELL_SIZE)*(y+0.5)/CELL_H);
			projs[y*CELL_W+x] = ENT_BOX(PROJECTILE(
				.pos = topleft + offset,
				.proto = pp_bigball,
				.flags = PFLAG_NOCLEAR,
				.color = RGBA(1,0.2,0.5,0),
			));
		}
	}
	for(;;) {
		WAIT_EVENT_OR_DIE(ARGS.destroy);
		if(*ARGS.clear_dir == CD_NONE) { // just shrink projectiles
			play_sfx("warp");
			for(int i = 0; i < CELL_W*CELL_H; i++) {
				Projectile *p = ENT_UNBOX(projs[i]);
				if(p != NULL) {
					projectile_set_prototype(p, pp_flea);
				}
			}
			play_sfx("warp");
			WAIT(CLEAR_TIME+40);
			for(int i = 0; i < CELL_W*CELL_H; i++) {
				Projectile *p = ENT_UNBOX(projs[i]);
				if(p != NULL) {
					projectile_set_prototype(p, pp_bigball);
				}
			}
		} else {
			CellDirection dir = *ARGS.clear_dir;

			for(int i = 0; i < CELL_W*CELL_H; i++) {
				real delay = 0.5 * CLEAR_TIME * cell_dir_transition(i, dir);
				Projectile *p = ENT_UNBOX(projs[i]);
				if(p != NULL) {
					INVOKE_TASK_DELAYED(delay, cell_proj_aim, ENT_BOX(p));
				}
			}
			break;
		}
	}
}

TASK(clearing_laser_rect, { int start_cell; int end_cell; int start_delay; int move_duration; int end_delay; bool collide; const Color *color; }) {
	play_sfx("laser1");
	cmplx topleft0 = cell_topleft(ARGS.start_cell);
	cmplx topleft1 = cell_topleft(ARGS.end_cell);

	cmplx diff = topleft1-topleft0;

	cmplx positions[] = {
		topleft0,
		topleft0 + creal(CELL_SIZE),
		topleft0 + CELL_SIZE,
		topleft0 + I * cimag(CELL_SIZE),
		topleft0,
	};

	real dur = ARGS.start_delay + ARGS.move_duration + ARGS.end_delay;

	real lwidth = 10;

	DECLARE_ENT_ARRAY(Laser, lasers, 4);
	for(int i = 0; i < 4; i++) {
		Laser *l = create_laserline_ab(positions[i], positions[i+1], lwidth, ARGS.collide ? ARGS.start_delay : (ARGS.start_delay+ARGS.move_duration), dur, ARGS.color);
		l->unclearable = true;

		ENT_ARRAY_ADD(&lasers, l);
	}
	WAIT(ARGS.start_delay);

	for(int i = 0; i < ARGS.move_duration; i++) {
		real f = (i+1)/(real)ARGS.move_duration;
		ENT_ARRAY_FOREACH_COUNTER(&lasers, int j, Laser *l, {
			l->pos = positions[j] + diff*glm_ease_quad_in(f);
		});
		YIELD;
	}
}

DEFINE_EXTERN_TASK(stagex_spell_mem_copy) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);

	COEVENTS_ARRAY(destroy[CELLS_W*CELLS_H]) cell_events;
	TASK_HOST_EVENTS(cell_events);

	int cell_gaps[CELLS_W*CELLS_H];

	for(int i = 0; i < ARRAY_SIZE(cell_gaps); i++) {
		cell_gaps[i] = -1;
	}
	WAIT(60);

	CellDirection clear_dir = CD_NONE;
	int plr_cell = find_player_cell();
	for(int x = 0; x < CELLS_W; x++) {
		for(int y = 0; y < CELLS_H; y++) {
			int idx = y*CELLS_W+x;
			if(idx == plr_cell) {
				continue;
			}
			cell_gaps[idx] = rng_irange(0, CELL_W * CELL_H);
			INVOKE_SUBTASK(spawn_cell, idx, cell_gaps[idx], &cell_events.destroy[idx], &clear_dir);
			WAIT(10);
		}
	}
	for(int step = 0;; step++) {
		WAIT(60);
		clear_dir = 0;
		coevent_signal(&cell_events.destroy[plr_cell]);

		clear_dir = cell_find_expansion_dir(plr_cell);
		int start_delay = 40;
		int end_delay = 240;
		INVOKE_SUBTASK(clearing_laser_rect,
			       .start_cell = plr_cell,
			       .end_cell = cell_apply_dir(plr_cell, clear_dir),
			       .start_delay = start_delay,
			       .move_duration = CLEAR_TIME,
			       .end_delay = end_delay,
			       .collide = true,
			       .color = RGBA(1,0,0,0.5)
		);
		WAIT(start_delay);


		int dest_cell = cell_apply_dir(plr_cell, clear_dir);
		coevent_signal(&cell_events.destroy[dest_cell]);
		cell_gaps[dest_cell] = -1;

		boss->move = move_towards(CMPLX(creal(cell_topleft(dest_cell)+CELL_SIZE*0.5), 100), 0.01);
		WAIT(CLEAR_TIME);

		int src_cell;
		do {
			src_cell = rng_irange(0, CELLS_W*CELLS_H);
		} while(cell_gaps[src_cell] == -1);

		INVOKE_SUBTASK(clearing_laser_rect,
			       .start_cell = dest_cell,
			       .end_cell = src_cell,
			       .start_delay = 0,
			       .move_duration = 20,
			       .end_delay = 240,
			       .color = RGBA(0,0,1,0.5)
		);

		INVOKE_SUBTASK_DELAYED(180, common_charge, boss->pos, RGBA(0.5, 0.6, 2.0, 0.0), 60, .sound = COMMON_CHARGE_SOUNDS);
		WAIT(240);

		cell_gaps[dest_cell] = cell_gaps[src_cell];
		INVOKE_SUBTASK(spawn_cell, dest_cell, cell_gaps[dest_cell], &cell_events.destroy[dest_cell], &clear_dir);
		if(step == 0) {
			cell_gaps[plr_cell] = cell_gaps[src_cell];
			INVOKE_SUBTASK(spawn_cell, plr_cell, cell_gaps[plr_cell], &cell_events.destroy[plr_cell], &clear_dir);
		}
		plr_cell = find_player_cell();
		WAIT(120);
	}
}
