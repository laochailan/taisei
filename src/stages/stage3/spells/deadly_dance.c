/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"
#include "../scuttle.h"

#include "common_tasks.h"
#include "global.h"

MODERNIZE_THIS_FILE_AND_REMOVE_ME

TASK(deadly_dance_proj, { BoxedProjectile p; int t; int i; }) {

    Projectile *p = TASK_BIND(ARGS.p);

    int t = ARGS.t;
    int i = ARGS.i;
    double a = (M_PI/(5 + global.diff) * i * 2);
    PROJECTILE(
        .proto = rng_chance(0.5) ? pp_thickrice : pp_rice,
        .pos = p->pos,
        .color = RGB(0.3, 0.7 + 0.3 * psin(a/3.0 + t/20.0), 0.3),
        .move = move_accelerated(0, 0.005 * cdir((M_PI * 2 * sin(a / 5.0 + t / 20.0))))
    );
}

DEFINE_EXTERN_TASK(stage3_spell_deadly_dance) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);

    aniplayer_queue(&boss->ani, "dance", 0);
    int intensity = difficulty_value(15, 18, 21, 24);

    int i;
    for(int time = 0; time < 1000; ++time) {
        WAIT(1);

        DECLARE_ENT_ARRAY(Projectile, projs, intensity*2);

        real angle_ofs = rng_f32() * M_PI * 2;
        double t = time * 1.5 * (0.4 + 0.3 * global.diff);
        double moverad = fmin(160, time/2.7);

        boss->pos = VIEWPORT_W/2 + VIEWPORT_H*I/2 + sin(t/50.0) * moverad * cdir(M_PI_2 * t/100.0);

        if(!(time % 70)) {
            for(i = 0; i < intensity; ++i) {
                double a = (M_PI/(5 + global.diff) * i * 2);
                ENT_ARRAY_ADD(&projs, PROJECTILE(
                    .proto = pp_wave,
                    .pos = boss->pos,
                    .color = RGB(0.3, 0.3 + 0.7 * psin((M_PI/(5 + global.diff) * i * 2) * 3 + time/50.0), 0.3),
                    .move = move_accelerated(0, 0.02 * cdir(angle_ofs + a + time/10.0)),
                ));
            }
            ENT_ARRAY_FOREACH(&projs, Projectile *p, {
                INVOKE_SUBTASK_DELAYED(150, deadly_dance_proj, ENT_BOX(p), t, i);
            });
        }

        if(global.diff > D_Easy && !(time % 35)) {
            int count = difficulty_value(2, 4, 6, 8);
            for(i = 0; i < count; ++i) {
                PROJECTILE(
                    .proto = pp_ball,
                    .pos = boss->pos,
                    .color = RGB(1.0, 1.0, 0.3),
                    .move = move_asymptotic_simple(
                        (0.5 + 3 * psin(time + M_PI / 3 * 2 * i)) * cdir(angle_ofs + time / 20.0 + M_PI / count * i * 2),
                        1.5
                    )
                );
            }
        }

        play_sound("shot1");

        if(!(time % 3)) {
            for(i = -1; i < 2; i += 2) {
                double c = psin(time/10.0);
                PROJECTILE(
                    .proto = pp_crystal,
                    .pos = boss->pos,
                    .color = RGBA_MUL_ALPHA(0.3 + c * 0.7, 0.6 - c * 0.3, 0.3, 0.7),
                    .move = move_linear(
                        10 * (cnormalize(global.plr.pos - boss->pos) + (M_PI/4.0 * i * (1 - time / 2500.0)) * (1 - 0.5 * psin(time / 15.0)))
                    )
                );
            }
        }
    }
}
