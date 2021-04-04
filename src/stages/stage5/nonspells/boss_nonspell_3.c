/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "nonspells.h"

MODERNIZE_THIS_FILE_AND_REMOVE_ME

void iku_bolts3(Boss *b, int time) {
	int t = time % 400;
	TIMER(&t);

	FROM_TO(0, 400, 2) {
		iku_nonspell_spawn_cloud();
	}

	FROM_TO(60, 400, 60) {
		aniplayer_queue(&b->ani, (_i&1) ? "dashdown_left" : "dashdown_right",1);
		aniplayer_queue(&b->ani, "main", 0);
		int i, c = 10+global.diff;
		cmplx n = cexp(I*carg(global.plr.pos-b->pos)+0.1*I-0.2*I*frand());
		for(i = 0; i < c; i++) {
			PROJECTILE(
				.proto = pp_ball,
				.pos = b->pos,
				.color = RGBA(0.4, 1.0, 1.0, 0.0),
				.rule = asymptotic,
				.args = {
					(i+2)*0.4*n+0.2*(global.diff-1)*frand(),
					3
				},
			);
		}

		play_sfx("shot2");
		play_sfx("redirect");
	}

	FROM_TO_SND("shot1_loop", 0, 400, 5-global.diff)
		if(frand() < 0.9) {
			PROJECTILE(
				.proto = pp_plainball,
				.pos = b->pos,
				.color = RGB(0.2,0,0.8),
				.rule = linear,
				.args = { cexp(0.1*I*_i) }
			);
		}

	FROM_TO(0, 70, 1) {
		GO_TO(b, 100+200.0*I, 0.02);
	}

	FROM_TO(230, 300, 1) {
		GO_TO(b, VIEWPORT_W-100+200.0*I, 0.02);
	}

}
