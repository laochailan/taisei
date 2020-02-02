/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "projectile.h"

#include "global.h"
#include "list.h"
#include "stageobjects.h"
#include "util/glm.h"

static ht_ptr2int_t shader_sublayer_map;

static ProjArgs defaults_proj = {
	.sprite = "proj/",
	.dest = &global.projs,
	.type = PROJ_ENEMY,
	.damage_type = DMG_ENEMY_SHOT,
	.color = RGB(1, 1, 1),
	.blend = BLEND_PREMUL_ALPHA,
	.shader = "sprite_bullet",
	.layer = LAYER_BULLET,
};

static ProjArgs defaults_part = {
	.sprite = "part/",
	.dest = &global.particles,
	.type = PROJ_PARTICLE,
	.damage_type = DMG_UNDEFINED,
	.color = RGB(1, 1, 1),
	.blend = BLEND_PREMUL_ALPHA,
	.shader = "sprite_particle",
	.layer = LAYER_PARTICLE_MID,
};

static void process_projectile_args(ProjArgs *args, ProjArgs *defaults) {
	if(args->proto && args->proto->process_args) {
		args->proto->process_args(args->proto, args);
		return;
	}

	// TODO: move this stuff into prototypes along with the defaults?

	if(args->sprite) {
		args->sprite_ptr = prefix_get_sprite(args->sprite, defaults->sprite);
	}

	if(!args->shader_ptr) {
		if(args->shader) {
			args->shader_ptr = r_shader_get(args->shader);
		} else {
			args->shader_ptr = defaults->shader_ptr;
		}
	}

	if(!args->draw_rule.func) {
		args->draw_rule = pdraw_basic();
	}

	if(!args->blend) {
		args->blend = defaults->blend;
	}

	if(!args->dest) {
		args->dest = defaults->dest;
	}

	if(!args->type) {
		args->type = defaults->type;
	}

	if(!args->color) {
		args->color = defaults->color;
	}

	if(!args->max_viewport_dist && (args->type == PROJ_PARTICLE || args->type == PROJ_PLAYER)) {
		args->max_viewport_dist = 300;
	}

	if(!args->layer) {
		if(args->type == PROJ_PLAYER) {
			args->layer = LAYER_PLAYER_SHOT;
		} else {
			args->layer = defaults->layer;
		}
	}

	if(args->damage_type == DMG_UNDEFINED) {
		args->damage_type = defaults->damage_type;

		if(args->type == PROJ_PLAYER && args->damage_type == DMG_ENEMY_SHOT) {
			args->damage_type = DMG_PLAYER_SHOT;
		}
	}

	if(args->scale == 0) {
		args->scale = 1+I;
	} else if(cimagf(args->scale) == 0) {
		args->scale = CMPLXF(crealf(args->scale), crealf(args->scale));
	}

	if(args->opacity == 0) {
		args->opacity = 1;
	}

	assert(args->type <= PROJ_PLAYER);
}

static void projectile_size(Projectile *p, double *w, double *h) {
	if(p->type == PROJ_PARTICLE && p->sprite != NULL) {
		*w = p->sprite->w;
		*h = p->sprite->h;
	} else {
		*w = creal(p->size);
		*h = cimag(p->size);
	}

	assert(*w > 0);
	assert(*h > 0);
}

static void ent_draw_projectile(EntityInterface *ent);

static inline char* event_name(int ev) {
	switch(ev) {
		case EVENT_BIRTH: return "EVENT_BIRTH";
		case EVENT_DEATH: return "EVENT_DEATH";
		default: UNREACHABLE;
	}
}

static Projectile* spawn_bullet_spawning_effect(Projectile *p);

static inline int proj_call_rule(Projectile *p, int t) {
	int result = ACTION_NONE;

	if(p->timeout > 0 && t >= p->timeout) {
		result = ACTION_DESTROY;
	} else if(p->rule != NULL) {
		result = p->rule(p, t);

		if(t < 0 && result != ACTION_ACK) {
			set_debug_info(&p->debug);
			log_fatal(
				"Projectile rule didn't acknowledge %s (returned %i, expected %i)",
				event_name(t),
				result,
				ACTION_ACK
			);
		}
	} else if(t >= 0) {
		if(!(p->flags & PFLAG_NOMOVE)) {
			move_update(&p->pos, &p->move);
		}

		if(!(p->flags & PFLAG_MANUALANGLE)) {
			cmplx delta_pos = p->pos - p->prevpos;

			if(delta_pos) {
				p->angle = carg(delta_pos);
			}
		}
	}

	if(/*t == 0 ||*/ t == EVENT_BIRTH) {
		p->prevpos = p->pos;
	}

	if(t == 0) {
		spawn_bullet_spawning_effect(p);
	}

	return result;
}

void projectile_set_prototype(Projectile *p, ProjPrototype *proto) {
	if(p->proto && p->proto->deinit_projectile) {
		p->proto->deinit_projectile(p->proto, p);
	}

	if(proto && proto->init_projectile) {
		proto->init_projectile(proto, p);
	}

	p->proto = proto;
}

cmplx projectile_graze_size(Projectile *p) {
	if(
		p->type == PROJ_ENEMY &&
		!(p->flags & (PFLAG_NOGRAZE | PFLAG_NOCOLLISION)) &&
		p->graze_counter < 3 &&
		global.frames >= p->graze_cooldown
	) {
		cmplx s = (p->size * 420 /* graze it */) / (2 * p->graze_counter + 1);
		return sqrt(creal(s)) + sqrt(cimag(s)) * I;
	}

	return 0;
}

float32 projectile_timeout_factor(Projectile *p) {
	return p->timeout ? (global.frames - p->birthtime) / p->timeout : 0;
}

static double projectile_rect_area(Projectile *p) {
	double w, h;
	projectile_size(p, &w, &h);
	return w * h;
}

void projectile_set_layer(Projectile *p, drawlayer_t layer) {
	if(!(layer & LAYER_LOW_MASK)) {
		drawlayer_low_t sublayer;

		switch(p->type) {
			case PROJ_ENEMY:
				// 1. Large projectiles go below smaller ones.
				sublayer = LAYER_LOW_MASK - (drawlayer_low_t)projectile_rect_area(p);
				sublayer = (sublayer << 4) & LAYER_LOW_MASK;
				// 2. Group by shader (hardcoded precedence).
				sublayer |= ht_get(&shader_sublayer_map, p->shader, 0) & 0xf;
				// If specific blending order is required, then you should set up the sublayer manually.
				layer |= sublayer;
				break;

			case PROJ_PARTICLE:
				// 1. Group by shader (hardcoded precedence).
				sublayer = ht_get(&shader_sublayer_map, p->shader, 0) & 0xf;
				sublayer <<= 4;
				sublayer |= 0x100;
				// If specific blending order is required, then you should set up the sublayer manually.
				layer |= sublayer;
				break;

			default:
				break;
		}
	}

	p->ent.draw_layer = layer;
}

static Projectile* _create_projectile(ProjArgs *args) {
	if(IN_DRAW_CODE) {
		log_fatal("Tried to spawn a projectile while in drawing code");
	}

	Projectile *p = (Projectile*)objpool_acquire(stage_object_pools.projectiles);

	p->birthtime = global.frames;
	p->pos = p->pos0 = p->prevpos = args->pos;
	p->angle = args->angle;
	p->rule = args->rule;
	p->draw_rule = args->draw_rule;
	p->shader = args->shader_ptr;
	p->blend = args->blend;
	p->sprite = args->sprite_ptr;
	p->type = args->type;
	p->color = *args->color;
	p->max_viewport_dist = args->max_viewport_dist;
	p->size = args->size;
	p->collision_size = args->collision_size;
	p->flags = args->flags;
	p->timeout = args->timeout;
	p->damage = args->damage;
	p->damage_type = args->damage_type;
	p->clear_flags = 0;
	p->move = args->move;
	p->scale = args->scale;
	p->opacity = args->opacity;

	memcpy(p->args, args->args, sizeof(p->args));

	p->ent.draw_func = ent_draw_projectile;

	projectile_set_prototype(p, args->proto);

	// p->collision_size *= 10;
	// p->size *= 5;

	if((p->type == PROJ_ENEMY || p->type == PROJ_PLAYER) && (creal(p->size) <= 0 || cimag(p->size) <= 0)) {
		log_fatal("Tried to spawn a projectile with invalid size %f x %f", creal(p->size), cimag(p->size));
	}

	projectile_set_layer(p, args->layer);

	coevent_init(&p->events.killed);
	ent_register(&p->ent, ENT_PROJECTILE);

	// TODO: Maybe allow ACTION_DESTROY here?
	// But in that case, code that uses this function's return value must be careful to not dereference a NULL pointer.
	proj_call_rule(p, EVENT_BIRTH);
	alist_append(args->dest, p);

	return p;
}

Projectile* create_projectile(ProjArgs *args) {
	process_projectile_args(args, &defaults_proj);
	return _create_projectile(args);
}

Projectile* create_particle(ProjArgs *args) {
	process_projectile_args(args, &defaults_part);
	return _create_projectile(args);
}

#ifdef PROJ_DEBUG
Projectile* _proj_attach_dbginfo(Projectile *p, DebugInfo *dbg, const char *callsite_str) {
	// log_debug("Spawn: [%s]", callsite_str);
	memcpy(&p->debug, dbg, sizeof(DebugInfo));
	set_debug_info(dbg);
	return p;
}
#endif

static void *_delete_projectile(ListAnchor *projlist, List *proj, void *arg) {
	Projectile *p = (Projectile*)proj;
	proj_call_rule(p, EVENT_DEATH);
	coevent_signal_once(&p->events.killed);
	ent_unregister(&p->ent);
	objpool_release(stage_object_pools.projectiles, alist_unlink(projlist, proj));
	return NULL;
}

static void delete_projectile(ProjectileList *projlist, Projectile *proj) {
	_delete_projectile((ListAnchor*)projlist, (List*)proj, NULL);
}

void delete_projectiles(ProjectileList *projlist) {
	alist_foreach(projlist, _delete_projectile, NULL);
}

void calc_projectile_collision(Projectile *p, ProjCollisionResult *out_col) {
	assert(out_col != NULL);

	out_col->type = PCOL_NONE;
	out_col->entity = NULL;
	out_col->fatal = false;
	out_col->location = p->pos;
	out_col->damage.amount = p->damage;
	out_col->damage.type = p->damage_type;

	if(p->flags & PFLAG_NOCOLLISION) {
		goto skip_collision;
	}

	if(p->type == PROJ_ENEMY) {
		Ellipse e_proj = {
			.axes = p->collision_size,
			.angle = p->angle + M_PI/2,
		};

		LineSegment seg = {
			.a = global.plr.pos - global.plr.velocity - p->prevpos,
			.b = global.plr.pos - p->pos
		};

		attr_unused double seglen = cabs(seg.a - seg.b);

		if(seglen > 30) {
			log_debug(
				seglen > VIEWPORT_W
					? "Lerp over HUGE distance %f; this is ABSOLUTELY a bug! Player speed was %f. Spawned at %s:%d (%s); proj time = %d"
					: "Lerp over large distance %f; this is either a bug or a very fast projectile, investigate. Player speed was %f. Spawned at %s:%d (%s); proj time = %d",
				seglen,
				cabs(global.plr.velocity),
				p->debug.file,
				p->debug.line,
				p->debug.func,
				global.frames - p->birthtime
			);
		}

		if(lineseg_ellipse_intersect(seg, e_proj)) {
			out_col->type = PCOL_ENTITY;
			out_col->entity = &global.plr.ent;
			out_col->fatal = true;
		} else {
			e_proj.axes = projectile_graze_size(p);

			if(creal(e_proj.axes) > 1 && lineseg_ellipse_intersect(seg, e_proj)) {
				out_col->type = PCOL_PLAYER_GRAZE;
				out_col->entity = &global.plr.ent;
				out_col->location = p->pos;
			}
		}
	} else if(p->type == PROJ_PLAYER) {
		for(Enemy *e = global.enemies.first; e; e = e->next) {
			if(e->hp != ENEMY_IMMUNE && cabs(e->pos - p->pos) < 30) {
				out_col->type = PCOL_ENTITY;
				out_col->entity = &e->ent;
				out_col->fatal = true;

				return;
			}
		}

		if(global.boss && cabs(global.boss->pos - p->pos) < 42) {
			if(boss_is_vulnerable(global.boss)) {
				out_col->type = PCOL_ENTITY;
				out_col->entity = &global.boss->ent;
				out_col->fatal = true;
			}
		}
	}

skip_collision:

	if(out_col->type == PCOL_NONE && !projectile_in_viewport(p)) {
		out_col->type = PCOL_VOID;
		out_col->fatal = true;
	}
}

void apply_projectile_collision(ProjectileList *projlist, Projectile *p, ProjCollisionResult *col) {
	switch(col->type) {
		case PCOL_NONE:
		case PCOL_VOID:
			break;

		case PCOL_PLAYER_GRAZE: {
			player_graze(ENT_CAST(col->entity, Player), col->location, 10 + 10 * p->graze_counter, 3 + p->graze_counter, &p->color);

			p->graze_counter++;
			p->graze_cooldown = global.frames + 12;
			p->graze_counter_reset_timer = global.frames;

			break;
		}

		case PCOL_ENTITY: {
			ent_damage(col->entity, &col->damage);
			break;
		}

		default:
			UNREACHABLE;
	}

	if(col->fatal) {
		delete_projectile(projlist, p);
	}
}

static void ent_draw_projectile(EntityInterface *ent) {
	Projectile *proj = ENT_CAST(ent, Projectile);

	r_blend(proj->blend);
	r_shader_ptr(proj->shader);

#ifdef PROJ_DEBUG
	static Projectile prev_state;
	memcpy(&prev_state, proj, sizeof(Projectile));

	proj->draw_rule.func(proj, global.frames - proj->birthtime, proj->draw_rule.args);

	if(memcmp(&prev_state, proj, sizeof(Projectile))) {
		set_debug_info(&proj->debug);
		log_fatal("Projectile modified its state in draw rule");
	}
#else
	proj->draw_rule.func(proj, global.frames - proj->birthtime, proj->draw_rule.args);
#endif
}

bool projectile_in_viewport(Projectile *proj) {
	double w, h;
	int e = proj->max_viewport_dist;
	projectile_size(proj, &w, &h);

	return !(creal(proj->pos) + w/2 + e < 0 || creal(proj->pos) - w/2 - e > VIEWPORT_W
		  || cimag(proj->pos) + h/2 + e < 0 || cimag(proj->pos) - h/2 - e > VIEWPORT_H);
}

Projectile* spawn_projectile_collision_effect(Projectile *proj) {
	if(proj->flags & PFLAG_NOCOLLISIONEFFECT) {
		return NULL;
	}

	if(proj->sprite == NULL) {
		return NULL;
	}

	return PARTICLE(
		.sprite_ptr = proj->sprite,
		.size = proj->size,
		.pos = proj->pos,
		.color = &proj->color,
		.flags = proj->flags | PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE,
		.layer = LAYER_PARTICLE_HIGH,
		.shader_ptr = proj->shader,
		.draw_rule = pdraw_timeout_scale(2+I, 0+I),
		.angle = proj->angle,
		// .rule = linear,
		// .args = { 5*cexp(I*proj->angle) },
		.move = { .velocity = 5*cexp(I*proj->angle), .retention = 0.95 },
		.timeout = 10,
	);
}

static void really_clear_projectile(ProjectileList *projlist, Projectile *proj) {
	spawn_projectile_clear_effect(proj);

	if(!(proj->flags & PFLAG_NOCLEARBONUS)) {
		create_clear_item(proj->pos, proj->clear_flags);
	}

	delete_projectile(projlist, proj);
}

bool clear_projectile(Projectile *proj, uint flags) {
	switch(proj->type) {
		case PROJ_PLAYER:
		case PROJ_PARTICLE:
			return false;

		default: break;
	}

	if(!(flags & CLEAR_HAZARDS_FORCE) && !projectile_is_clearable(proj)) {
		return false;
	}

	proj->type = PROJ_DEAD;
	proj->clear_flags |= flags;

	return true;
}

void kill_projectile(Projectile* proj) {
	proj->flags |= PFLAG_INTERNAL_DEAD | PFLAG_NOCOLLISION | PFLAG_NOCLEAR;
	proj->ent.draw_layer = LAYER_NODRAW;
	// WARNING: must be done last, an event handler may cancel the task this function is running in!
	coevent_signal_once(&proj->events.killed);
}

void process_projectiles(ProjectileList *projlist, bool collision) {
	ProjCollisionResult col = { 0 };

	int action;
	bool stage_cleared = stage_is_cleared();

	for(Projectile *proj = projlist->first, *next; proj; proj = next) {
		next = proj->next;
		proj->prevpos = proj->pos;

		if(proj->flags & PFLAG_INTERNAL_DEAD) {
			delete_projectile(projlist, proj);
			continue;
		}

		if(stage_cleared) {
			clear_projectile(proj, CLEAR_HAZARDS_BULLETS | CLEAR_HAZARDS_FORCE);
		}

		action = proj_call_rule(proj, global.frames - proj->birthtime);

		if(proj->graze_counter && proj->graze_counter_reset_timer - global.frames <= -90) {
			proj->graze_counter--;
			proj->graze_counter_reset_timer = global.frames;
		}

		if(proj->type == PROJ_DEAD && !(proj->clear_flags & CLEAR_HAZARDS_NOW)) {
			proj->clear_flags |= CLEAR_HAZARDS_NOW;
		}

		if(action == ACTION_DESTROY) {
			memset(&col, 0, sizeof(col));
			col.fatal = true;
		} else if(collision) {
			calc_projectile_collision(proj, &col);

			if(col.fatal && col.type != PCOL_VOID) {
				spawn_projectile_collision_effect(proj);
			}
		} else {
			memset(&col, 0, sizeof(col));

			if(!projectile_in_viewport(proj)) {
				col.fatal = true;
			}
		}

		apply_projectile_collision(projlist, proj, &col);
	}

	for(Projectile *proj = projlist->first, *next; proj; proj = next) {
		next = proj->next;

		if(proj->type == PROJ_DEAD && (proj->clear_flags & CLEAR_HAZARDS_NOW)) {
			really_clear_projectile(projlist, proj);
		}
	}
}

int trace_projectile(Projectile *p, ProjCollisionResult *out_col, ProjCollisionType stopflags, int timeofs) {
	int t;

	for(t = timeofs; p; ++t) {
		int action = p->rule(p, t);
		calc_projectile_collision(p, out_col);

		if(out_col->type & stopflags || action == ACTION_DESTROY) {
			return t;
		}
	}

	return t;
}

bool projectile_is_clearable(Projectile *p) {
	if(p->type == PROJ_DEAD) {
		return true;
	}

	if(p->type == PROJ_ENEMY) {
		return (p->flags & PFLAG_NOCLEAR) != PFLAG_NOCLEAR;
	}

	return false;
}

int projectile_time(Projectile *p) {
	return global.frames - p->birthtime;
}

int linear(Projectile *p, int t) { // sure is physics in here; a[0]: velocity
	if(t == EVENT_DEATH) {
		return ACTION_ACK;
	}

	p->angle = carg(p->args[0]);

	if(t == EVENT_BIRTH) {
		return ACTION_ACK;
	}

	p->pos = p->pos0 + p->args[0]*t;

	return ACTION_NONE;
}

int accelerated(Projectile *p, int t) {
	if(t == EVENT_DEATH) {
		return ACTION_ACK;
	}

	p->angle = carg(p->args[0]);

	if(t == EVENT_BIRTH) {
		return ACTION_ACK;
	}

	p->pos += p->args[0];
	p->args[0] += p->args[1];

	return 1;
}

int asymptotic(Projectile *p, int t) { // v = a[0]*(a[1] + 1); a[1] -> 0
	if(t == EVENT_DEATH) {
		return ACTION_ACK;
	}

	p->angle = carg(p->args[0]);

	if(t == EVENT_BIRTH) {
		return ACTION_ACK;
	}

	p->args[1] *= 0.8;
	p->pos += p->args[0]*(p->args[1] + 1);

	return 1;
}

static inline bool proj_uses_spawning_effect(Projectile *proj, ProjFlags effect_flag) {
	if(proj->type != PROJ_ENEMY) {
		return false;
	}

	if((proj->flags & effect_flag) == effect_flag) {
		return false;
	}

	return true;
}

static float proj_spawn_effect_factor(Projectile *proj, int t) {
	static const int maxt = 16;

	if(t >= maxt || !proj_uses_spawning_effect(proj, PFLAG_NOSPAWNFADE)) {
		return 1;
	}

	return t / (float)maxt;
}

static inline void apply_common_transforms(Projectile *proj, int t) {
	r_mat_mv_translate(creal(proj->pos), cimag(proj->pos), 0);
	r_mat_mv_rotate(proj->angle + M_PI/2, 0, 0, 1);

	/*
	float s = 0.75 + 0.25 * proj_spawn_effect_factor(proj, t);

	if(s != 1) {
		r_mat_mv_scale(s, s, 1);
	}
	*/
}

static void bullet_highlight_draw(Projectile *p, int t, ProjDrawRuleArgs args) {
	float timefactor = t / p->timeout;
	float sx = args[0].as_float[0];
	float sy = args[0].as_float[1];
	float tex_angle = args[1].as_float[0];

	float opacity = pow(1 - timefactor, 2);
	opacity = min(1, 1.5 * opacity) * min(1, timefactor * 10);
	opacity *= p->opacity;

	r_mat_tex_push();
	r_mat_tex_translate(0.5, 0.5, 0);
	r_mat_tex_rotate(tex_angle, 0, 0, 1);
	r_mat_tex_translate(-0.5, -0.5, 0);

	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = p->sprite,
		.shader_ptr = p->shader,
		.shader_params = &(ShaderCustomParams) {{ opacity }},
		.color = &p->color,
		.scale = { .x = sx, .y = sy },
		.rotation.angle = p->angle + M_PI * 0.5,
		.pos = { creal(p->pos), cimag(p->pos) }
	});

	r_mat_tex_pop();
}

static Projectile* spawn_projectile_highlight_effect_internal(Projectile *p, bool flare) {
	if(!p->sprite) {
		return NULL;
	}

	Color clr = p->color;
	clr.r = fmax(0.1, clr.r);
	clr.g = fmax(0.1, clr.g);
	clr.b = fmax(0.1, clr.b);
	float h, s, l;
	color_get_hsl(&clr, &h, &s, &l);
	s = s > 0 ? 0.75 : 0;
	l = 0.5;
	color_hsla(&clr, h, s, l, 0.05);

	float sx, sy;

	if(flare) {
		sx = pow(p->sprite->w, 0.7);
		sy = pow(p->sprite->h, 0.7);

		RNG_ARRAY(R, 5);

		PARTICLE(
			.sprite = "stardust_green",
			.shader = "sprite_bullet",
			.size = p->size * 4.5,
			.layer = LAYER_PARTICLE_HIGH | 0x40,
			.draw_rule = pdraw_timeout_scalefade_exp(0, 0.2f * fmaxf(sx, sy) * vrng_f32_range(R[0], 0.8f, 1.0f), 1, 0, 2),
			.angle = vrng_angle(R[1]),
			.pos = p->pos + vrng_range(R[2], 0, 8) * vrng_dir(R[3]),
			.flags = PFLAG_NOREFLECT,
			.timeout = vrng_range(R[4], 22, 26),
			.color = &clr,
		);
	}

	sx = pow((1.5 * p->sprite->w + 0.5 * p->sprite->h) * 0.5, 0.65);
	sy = pow((1.5 * p->sprite->h + 0.5 * p->sprite->w) * 0.5, 0.65);
	clr.a = 0.2;

	RNG_ARRAY(R, 5);

	return PARTICLE(
		.sprite = "bullet_cloud",
		.size = p->size * 4.5,
		.shader = "sprite_bullet",
		.layer = LAYER_PARTICLE_HIGH | 0x80,
		.draw_rule = {
			bullet_highlight_draw,
			.args[0].as_cmplx = 0.125 * (sx + I * sy),
			.args[1].as_float = vrng_angle(R[0]),
		},
		.angle = p->angle,
		.pos = p->pos + vrng_range(R[1], 0, 5) * vrng_dir(R[2]),
		.flags = PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE,
		.timeout = vrng_range(R[3], 30, 34),
		.color = &clr,
	);
}

Projectile* spawn_projectile_highlight_effect(Projectile *p) {
	return spawn_projectile_highlight_effect_internal(p, true);
}

static Projectile* spawn_bullet_spawning_effect(Projectile *p) {
	if(proj_uses_spawning_effect(p, PFLAG_NOSPAWNFLARE)) {
		return spawn_projectile_highlight_effect(p);
	}

	return NULL;
}

static void projectile_clear_effect_draw(Projectile *p, int t, ProjDrawRuleArgs args) {
	float o_tf = projectile_timeout_factor(p);
	float tf = glm_ease_circ_out(o_tf);

	Animation *ani = args[0].as_ptr;
	AniSequence *seq = args[1].as_ptr;
	float angle = args[2].as_float[0];
	float scale = args[2].as_float[1];

	SpriteParamsBuffer spbuf;
	SpriteParams sp = projectile_sprite_params(p, &spbuf);

	float o = spbuf.shader_params.vector[0];
	spbuf.shader_params.vector[0] = o * fmaxf(0, 1.5 * (1 - tf) - 0.5);

	r_draw_sprite(&sp);

	sp.sprite_ptr = animation_get_frame(ani, seq, o_tf * (seq->length - 1));
	sp.scale.as_cmplx *= scale * (0.0 + 1.5*tf);
	spbuf.color.a *= (1 - tf);
	spbuf.shader_params.vector[0] = o;
	sp.rotation.angle += t * 0.5*0 + angle;

	r_draw_sprite(&sp);
}

Projectile *spawn_projectile_clear_effect(Projectile *proj) {
	if((proj->flags & PFLAG_NOCLEAREFFECT) || proj->sprite == NULL) {
		return NULL;
	}

	cmplx v = proj->move.velocity;
	if(!v) {
		v = proj->pos - proj->prevpos;
	}

	Animation *ani = get_ani("part/bullet_clear");
	AniSequence *seq = get_ani_sequence(ani, "main");

	Sprite *sprite_ref = animation_get_frame(ani, seq, 0);
	float scale = fmaxf(proj->sprite->w, proj->sprite->h) / sprite_ref->w;

	return PARTICLE(
		.sprite_ptr = proj->sprite,
		.size = proj->size,
		.pos = proj->pos,
		.color = &proj->color,
		.flags = proj->flags | PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE,
		.shader_ptr = proj->shader,
		.draw_rule = {
			projectile_clear_effect_draw,
			.args[0].as_ptr = ani,
			.args[1].as_ptr = seq,
			.args[2].as_float = { rng_angle(), scale },
		},
		.angle = proj->angle,
		.opacity = proj->opacity,
		.scale = proj->scale,
		.timeout = seq->length - 1,
		.layer = LAYER_PARTICLE_BULLET_CLEAR,
		.move = move_asymptotic(v, 0, 0.85),
	);
}

SpriteParams projectile_sprite_params(Projectile *proj, SpriteParamsBuffer *spbuf) {
	spbuf->color = proj->color;
	spbuf->shader_params = (ShaderCustomParams) {{ proj->opacity, 0, 0, 0 }};

	SpriteParams sp = { 0 };
	sp.blend = proj->blend;
	sp.color = &spbuf->color;
	sp.pos.x = creal(proj->pos);
	sp.pos.y = cimag(proj->pos);
	sp.rotation = (SpriteRotationParams) {
		.angle = proj->angle + (float)(M_PI/2),
		.vector = { 0, 0, 1 },
	};
	sp.scale.x = crealf(proj->scale);
	sp.scale.y = cimagf(proj->scale);
	sp.shader_params = &spbuf->shader_params;
	sp.shader_ptr = proj->shader;
	sp.sprite_ptr = proj->sprite;

	return sp;
}

static void projectile_draw_sprite(Sprite *s, const Color *clr, float32 opacity, cmplx32 scale) {
	if(opacity <= 0 || crealf(scale) == 0) {
		return;
	}

	ShaderCustomParams p = {
		opacity,
	};

	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = s,
		.color = clr,
		.shader_params = &p,
		.scale = { crealf(scale), cimagf(scale) },
	});
}

void ProjDrawCore(Projectile *proj, const Color *c) {
	projectile_draw_sprite(proj->sprite, c, proj->opacity, proj->scale);
}

void ProjDraw(Projectile *proj, int t, ProjDrawRuleArgs args) {
	SpriteParamsBuffer spbuf;
	SpriteParams sp = projectile_sprite_params(proj, &spbuf);

	float eff = proj_spawn_effect_factor(proj, t);

	if(eff < 1) {
		spbuf.color.a *= eff;
		spbuf.shader_params.vector[0] *= fminf(1.0f, eff * 2.0f);
	}

	r_draw_sprite(&sp);
}

ProjDrawRule pdraw_basic(void) {
	return (ProjDrawRule) { ProjDraw };
}

static void pdraw_blast_func(Projectile *p, int t, ProjDrawRuleArgs args) {
	vec3 rot_axis = {
		args[0].as_float[0],
		args[0].as_float[1],
		args[1].as_float[0],
	};

	float32 rot_angle = args[1].as_float[1];
	float32 secondary_scale = args[2].as_float[0];

	float32 tf = projectile_timeout_factor(p);
	float32 opacity = (1.0f - tf) * p->opacity;

	if(tf <= 0 || opacity <= 0) {
		return;
	}

	SpriteParamsBuffer spbuf;
	SpriteParams sp = projectile_sprite_params(p, &spbuf);
	sp.rotation.angle = rot_angle;
	glm_vec3_copy(rot_axis, sp.rotation.vector);
	sp.scale.x = tf;
	sp.scale.y = tf;

	spbuf.color = *RGBA(0.3, 0.6, 1.0, 1);
	spbuf.shader_params.vector[0] = opacity;

	r_disable(RCAP_CULL_FACE);
	r_draw_sprite(&sp);
	sp.scale.as_cmplx *= secondary_scale;
	spbuf.color.a = 0;
	r_draw_sprite(&sp);
}

ProjDrawRule pdraw_blast(void) {
	float32 rot_angle = rng_f32_angle();
	float32 x = rng_f32();
	float32 y = rng_f32();
	float32 z = rng_f32();
	float32 secondary_scale = rng_f32_range(0.5, 1.5);

	vec3 rot_axis = { x, y, z };
	glm_vec3_normalize(rot_axis);

	return (ProjDrawRule) {
		.func = pdraw_blast_func,
		.args[0].as_float = { rot_axis[0], rot_axis[1] },
		.args[1].as_float = { rot_axis[2], rot_angle },
		.args[2].as_float = { secondary_scale, },
	};
}

void Shrink(Projectile *p, int t, ProjDrawRuleArgs args) {
	r_mat_mv_push();
	apply_common_transforms(p, t);

	float s = 2.0-t/(double)p->timeout*2;
	if(s != 1) {
		r_mat_mv_scale(s, s, 1);
	}

	ProjDrawCore(p, &p->color);
	r_mat_mv_pop();
}

void GrowFade(Projectile *p, int t, ProjDrawRuleArgs args) {
	r_mat_mv_push();
	apply_common_transforms(p, t);

	set_debug_info(&p->debug);
	assert(p->timeout != 0);

	float s = t/(double)p->timeout*(1 + (creal(p->args[2])? p->args[2] : p->args[1]));
	if(s != 1) {
		r_mat_mv_scale(s, s, 1);
	}

	ProjDrawCore(p, color_mul_scalar(COLOR_COPY(&p->color), 1 - t/(double)p->timeout));
	r_mat_mv_pop();
}

void Fade(Projectile *p, int t, ProjDrawRuleArgs args) {
	r_mat_mv_push();
	apply_common_transforms(p, t);
	ProjDrawCore(p, color_mul_scalar(COLOR_COPY(&p->color), 1 - t/(double)p->timeout));
	r_mat_mv_pop();
}

static void ScaleFadeImpl(Projectile *p, int t, int fade_exponent) {
	r_mat_mv_push();
	apply_common_transforms(p, t);

	double scale_min = creal(p->args[2]);
	double scale_max = cimag(p->args[2]);
	double timefactor = t / (double)p->timeout;
	double scale = scale_min * (1 - timefactor) + scale_max * timefactor;
	double alpha = pow(fabs(1 - timefactor), fade_exponent);

	r_mat_mv_scale(scale, scale, 1);
	ProjDrawCore(p, color_mul_scalar(COLOR_COPY(&p->color), alpha));
	r_mat_mv_pop();
}

void ScaleFade(Projectile *p, int t, ProjDrawRuleArgs args) {
	ScaleFadeImpl(p, t, 1);
}

void ScaleSquaredFade(Projectile *p, int t, ProjDrawRuleArgs args) {
	ScaleFadeImpl(p, t, 2);
}

static void pdraw_scalefade_func(Projectile *p, int t, ProjDrawRuleArgs args) {
	cmplx32 scale0 = args[0].as_cmplx;
	cmplx32 scale1 = args[1].as_cmplx;
	float32 opacity0 = args[2].as_float[0];
	float32 opacity1 = args[2].as_float[1];
	float32 opacity_exp = args[3].as_float[0];

	float32 timefactor = t / p->timeout;

	cmplx32 scale = clerpf(scale0, scale1, timefactor);
	float32 opacity = lerpf(opacity0, opacity1, timefactor);
	opacity = powf(opacity, opacity_exp);

	SpriteParamsBuffer spbuf;
	SpriteParams sp = projectile_sprite_params(p, &spbuf);
	spbuf.shader_params.vector[0] *= opacity;
	sp.scale.as_cmplx = cwmulf(sp.scale.as_cmplx, scale);

	r_draw_sprite(&sp);
}

ProjDrawRule pdraw_timeout_scalefade_exp(cmplx32 scale0, cmplx32 scale1, float32 opacity0, float32 opacity1, float32 opacity_exp) {
	if(cimagf(scale0) == 0) {
		scale0 = CMPLXF(crealf(scale0), crealf(scale0));
	}

	if(cimagf(scale1) == 0) {
		scale1 = CMPLXF(crealf(scale1), crealf(scale1));
	}

	return (ProjDrawRule) {
		.func = pdraw_scalefade_func,
		.args[0].as_cmplx = scale0,
		.args[1].as_cmplx = scale1,
		.args[2].as_float = { opacity0, opacity1 },
		.args[3].as_float = { opacity_exp },
	};
}

ProjDrawRule pdraw_timeout_scalefade(cmplx32 scale0, cmplx32 scale1, float32 opacity0, float32 opacity1) {
	return pdraw_timeout_scalefade_exp(scale0, scale1, opacity0, opacity1, 1.0f);
}

ProjDrawRule pdraw_timeout_scale(cmplx32 scale0, cmplx32 scale1) {
	// TODO: specialized code path without fade component
	return pdraw_timeout_scalefade(scale0, scale1, 1, 1);
}

ProjDrawRule pdraw_timeout_fade(float32 opacity0, float32 opacity1) {
	// TODO: specialized code path without scale component
	return pdraw_timeout_scalefade(1+I, 1+I, opacity0, opacity1);
}

static void pdraw_petal_func(Projectile *p, int t, ProjDrawRuleArgs args) {
	vec3 rot_axis = {
		args[0].as_float[0],
		args[0].as_float[1],
		args[1].as_float[0],
	};

	float32 rot_angle = args[1].as_float[1];

	SpriteParamsBuffer spbuf;
	SpriteParams sp = projectile_sprite_params(p, &spbuf);
	glm_vec3_copy(rot_axis, sp.rotation.vector);
	sp.rotation.angle = DEG2RAD*t*4.0f + rot_angle;

	spbuf.shader_params.vector[0] *= (1.0f - projectile_timeout_factor(p));

	r_disable(RCAP_CULL_FACE);
	r_draw_sprite(&sp);
}

ProjDrawRule pdraw_petal(float32 rot_angle, vec3 rot_axis) {
	glm_vec3_normalize(rot_axis);
	float32 x = rot_axis[0];
	float32 y = rot_axis[1];
	float32 z = rot_axis[2];

	return (ProjDrawRule) {
		.func = pdraw_petal_func,
		.args[0].as_float = { x, y },
		.args[1].as_float = { z, rot_angle },
	};
}

ProjDrawRule pdraw_petal_random(void) {
	float32 x = rng_f32();
	float32 y = rng_f32();
	float32 z = rng_f32();
	float32 rot_angle = rng_f32_angle();

	return pdraw_petal(rot_angle, (vec3) { x, y, z });
}

void petal_explosion(int n, cmplx pos) {
	for(int i = 0; i < n; i++) {
		cmplx v = rng_dir();
		v *= rng_range(3, 8);
		real t = rng_real();

		PARTICLE(
			.sprite = "petal",
			.pos = pos,
			.color = RGBA(sin(5*t) * t, cos(5*t) * t, 0.5 * t, 0),
			.rule = asymptotic,
			.move = move_asymptotic_simple(v, 5),
			.draw_rule = pdraw_petal_random(),
			// TODO: maybe remove this noreflect, there shouldn't be a cull mode mess anymore
			.flags = PFLAG_NOREFLECT | (n % 2 ? 0 : PFLAG_REQUIREDPARTICLE) | PFLAG_MANUALANGLE,
			.layer = LAYER_PARTICLE_PETAL,
		);
	}
}

void projectiles_preload(void) {
	const char *shaders[] = {
		// This array defines a shader-based fallback draw order
		"sprite_silhouette",
		defaults_proj.shader,
		defaults_part.shader,
	};

	const uint num_shaders = sizeof(shaders)/sizeof(*shaders);

	for(uint i = 0; i < num_shaders; ++i) {
		preload_resource(RES_SHADER_PROGRAM, shaders[i], RESF_PERMANENT);
	}

	// FIXME: Why is this here?
	preload_resources(RES_TEXTURE, RESF_PERMANENT,
		"part/lasercurve",
	NULL);

	// TODO: Maybe split this up into stage-specific preloads too?
	// some of these are ubiquitous, but some only appear in very specific parts.
	preload_resources(RES_SPRITE, RESF_PERMANENT,
		"part/blast",
		"part/bullet_cloud",
		"part/flare",
		"part/graze",
		"part/lightning0",
		"part/lightning1",
		"part/lightningball",
		"part/petal",
		"part/smoke",
		"part/smoothdot",
		"part/stain",
		"part/stardust_green",
	NULL);

	preload_resources(RES_ANIM, RESF_PERMANENT,
		"part/bullet_clear",
	NULL);

	preload_resources(RES_SFX, RESF_PERMANENT,
		"shot1",
		"shot2",
		"shot3",
		"shot1_loop",
		"shot_special1",
		"redirect",
	NULL);

	#define PP(name) (_pp_##name).preload(&_pp_##name);
	#include "projectile_prototypes/all.inc.h"

	ht_create(&shader_sublayer_map);

	for(uint i = 0; i < num_shaders; ++i) {
		ht_set(&shader_sublayer_map, r_shader_get(shaders[i]), i + 1);
	}

	defaults_proj.shader_ptr = r_shader_get(defaults_proj.shader);
	defaults_part.shader_ptr = r_shader_get(defaults_part.shader);
}

void projectiles_free(void) {
	ht_destroy(&shader_sublayer_map);
}
