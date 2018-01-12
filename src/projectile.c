/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "projectile.h"

#include <stdlib.h>
#include "global.h"
#include "list.h"
#include "vbo.h"
#include "stageobjects.h"

static ProjArgs defaults_proj = {
	.texture = "proj/",
	.draw_rule = ProjDraw,
	.dest = &global.projs,
	.type = EnemyProj,
	.color = RGB(1, 1, 1),
	.color_transform_rule = proj_clrtransform_bullet,
	.insertion_rule = proj_insert_sizeprio,
};

static ProjArgs defaults_part = {
	.texture = "part/",
	.draw_rule = ProjDraw,
	.dest = &global.particles,
	.type = Particle,
	.color = RGB(1, 1, 1),
	.color_transform_rule = proj_clrtransform_particle,
	.insertion_rule = list_append,
	// .insertion_rule = proj_insert_sizeprio,
};

static void process_projectile_args(ProjArgs *args, ProjArgs *defaults) {
	int texargs = (bool)args->texture + (bool)args->texture_ptr + (bool)args->size;

	if(texargs != 1) {
		log_fatal("Exactly one of .texture, .texture_ptr, or .size is required");
	}

	if(!args->rule) {
		log_fatal(".rule is required");
	}

	if(args->texture) {
		args->texture_ptr = prefix_get_tex(args->texture, defaults->texture);
	}

	if(!args->draw_rule) {
		args->draw_rule = ProjDraw;
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

	if(!args->color_transform_rule) {
		args->color_transform_rule = defaults->color_transform_rule;
	}

	if(!args->insertion_rule) {
		args->insertion_rule = defaults->insertion_rule;
	}

	if(!args->max_viewport_dist && (args->type == Particle || args->type >= PlrProj)) {
		args->max_viewport_dist = 300;
	}
}

static double projectile_rect_area(Projectile *p) {
	if(p->tex) {
		return p->tex->w * p->tex->h;
	} else {
		return creal(p->size) * cimag(p->size);
	}
}

static void projectile_size(Projectile *p, double *w, double *h) {
	if(p->tex) {
		*w = p->tex->w;
		*h = p->tex->h;
	} else {
		*w = creal(p->size);
		*h = cimag(p->size);
	}
}

static int projectile_sizeprio_func(List *vproj) {
	Projectile *proj = (Projectile*)vproj;

	if(proj->priority_override) {
		return proj->priority_override;
	}

	return -rint(projectile_rect_area(proj));
}

List* proj_insert_sizeprio(List **dest, List *elem) {
	return list_insert_at_priority_tail(dest, elem, projectile_sizeprio_func(elem), projectile_sizeprio_func);
}

static int projectile_colorprio_func(List *vproj) {
	Projectile *p = (Projectile*)vproj;
	Color c = p->color;
	uint32_t c32 = 0;
	float r, g, b, a;

	// convert color to 32bit
	parse_color(c, &r, &g, &b, &a);
	c32 |= (((c & CLRMASK_R) >> CLR_R) & 0xFF) << 0;
	c32 |= (((c & CLRMASK_G) >> CLR_G) & 0xFF) << 8;
	c32 |= (((c & CLRMASK_B) >> CLR_B) & 0xFF) << 16;

	return (int)c32;
}

List* proj_insert_colorprio(List **dest, List *elem) {
	return list_insert_at_priority_head(dest, elem, projectile_colorprio_func(elem), projectile_colorprio_func);
	// return list_push(dest, elem);
}

static Projectile* _create_projectile(ProjArgs *args) {
	if(IN_DRAW_CODE) {
		log_fatal("Tried to spawn a projectile while in drawing code");
	}

	Projectile *p = (Projectile*)objpool_acquire(stage_object_pools.projectiles);

	p->birthtime = global.frames;
	p->pos = p->pos0 = args->pos;
	p->angle = args->angle;
	p->rule = args->rule;
	p->draw_rule = args->draw_rule;
	p->color_transform_rule = args->color_transform_rule;
	p->tex = args->texture_ptr;
	p->type = args->type;
	p->color = args->color;
	p->grazed = (bool)(args->flags & PFLAG_NOGRAZE);
	p->max_viewport_dist = args->max_viewport_dist;
	p->size = args->size;
	p->flags = args->flags;
	p->priority_override = args->priority_override;

	memcpy(p->args, args->args, sizeof(p->args));

	// BUG: this currently breaks some projectiles
	//      enable this when they're fixed
	// assert(rule != NULL);
	// rule(p, EVENT_BIRTH);

	return (Projectile*)args->insertion_rule((List**)args->dest, (List*)p);
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

static void* _delete_projectile(List **projs, List *proj, void *arg) {
	Projectile *p = (Projectile*)proj;
	p->rule(p, EVENT_DEATH);

	del_ref(proj);
	objpool_release(stage_object_pools.projectiles, (ObjectInterface*)list_unlink(projs, proj));

	return NULL;
}

void delete_projectile(Projectile **projs, Projectile *proj) {
	_delete_projectile((List**)projs, (List*)proj, NULL);
}

void delete_projectiles(Projectile **projs) {
	list_foreach(projs, _delete_projectile, NULL);
}

void calc_projectile_collision(Projectile *p, ProjCollisionResult *out_col) {
	assert(out_col != NULL);

	out_col->type = PCOL_NONE;
	out_col->entity = NULL;
	out_col->fatal = false;
	out_col->location = p->pos;
	out_col->damage = 0;

	if(p->type == EnemyProj) {
		double w, h;
		projectile_size(p, &w, &h);

		double angle = carg(global.plr.pos - p->pos) + p->angle;
		double projr = sqrt(pow(w/2*cos(angle), 2) + pow(h/2*sin(angle), 2)) * 0.45;
		double grazer = max(w, h);
		double dst = cabs(global.plr.pos - p->pos);
		grazer = (0.9 * sqrt(grazer) + 0.1 * grazer) * 6;

		if(dst < projr + 1) {
			out_col->type = PCOL_PLAYER;
			out_col->entity = &global.plr;
			out_col->fatal = true;
		} else if(!p->grazed && dst < grazer && global.frames - abs(global.plr.recovery) > 0) {
			out_col->location = p->pos - grazer * 0.3 * cexp(I*carg(p->pos - global.plr.pos));
			out_col->type = PCOL_PLAYER_GRAZE;
			out_col->entity = &global.plr;
		}
	} else if(p->type >= PlrProj) {
		int damage = p->type - PlrProj;

		for(Enemy *e = global.enemies; e; e = e->next) {
			if(e->hp != ENEMY_IMMUNE && cabs(e->pos - p->pos) < 30) {
				out_col->type = PCOL_ENEMY;
				out_col->entity = e;
				out_col->fatal = true;
				out_col->damage = damage;

				return;
			}
		}

		if(global.boss && cabs(global.boss->pos - p->pos) < 42) {
			if(boss_is_vulnerable(global.boss)) {
				out_col->type = PCOL_BOSS;
				out_col->entity = global.boss;
				out_col->fatal = true;
				out_col->damage = damage;
			}
		}
	}

	if(out_col->type == PCOL_NONE && !projectile_in_viewport(p)) {
		out_col->type = PCOL_VOID;
		out_col->fatal = true;
	}
}

void apply_projectile_collision(Projectile **projlist, Projectile *p, ProjCollisionResult *col) {
	switch(col->type) {
		case PCOL_NONE: {
			break;
		}

		case PCOL_PLAYER: {
			if(global.frames - abs(((Player*)col->entity)->recovery) >= 0) {
				player_death(col->entity);
			}

			break;
		}

		case PCOL_PLAYER_GRAZE: {
			p->grazed = true;

			if(p->flags & PFLAG_GRAZESPAM) {
				player_graze(col->entity, col->location, 10, 2);
			} else {
				player_graze(col->entity, col->location, 50, 5);
			}

			break;
		}

		case PCOL_ENEMY: {
			Enemy *e = col->entity;
			player_add_points(&global.plr, col->damage * 0.5);

			#ifdef PLR_DPS_STATS
				global.plr.total_dmg += min(e->hp, col->damage);
			#endif

			e->hp -= col->damage;
			break;
		}

		case PCOL_BOSS: {
			if(boss_damage(col->entity, col->damage)) {
				player_add_points(&global.plr, col->damage * 0.2);

				#ifdef PLR_DPS_STATS
					global.plr.total_dmg += col->damage;
				#endif
			}
			break;
		}

		case PCOL_VOID: {
			break;
		}

		default: {
			log_fatal("Invalid collision type %x", col->type);
		}
	}

	if(col->fatal) {
		delete_projectile(projlist, p);
	}
}

void static_clrtransform_bullet(Color c, ColorTransform *out) {
	memcpy(out, (&(ColorTransform) {
		.B[0] = c & ~CLRMASK_A,
		.B[1] = rgba(1, 1, 1, 0),
		.A[1] = c &  CLRMASK_A,
	}), sizeof(ColorTransform));
}

void static_clrtransform_particle(Color c, ColorTransform *out) {
	memcpy(out, (&(ColorTransform) {
		.R[1] = c & CLRMASK_R,
		.G[1] = c & CLRMASK_G,
		.B[1] = c & CLRMASK_B,
		.A[1] = c & CLRMASK_A,
	}), sizeof(ColorTransform));
}

void proj_clrtransform_bullet(Projectile *p, int t, Color c, ColorTransform *out) {
	static_clrtransform_bullet(c, out);
}

void proj_clrtransform_particle(Projectile *p, int t, Color c, ColorTransform *out) {
	static_clrtransform_particle(c, out);
}

typedef enum ProjBlendMode {
	PBM_NORMAL,
	PBM_ADD,
	PBM_SUB,
} ProjBlendMode;

static inline void draw_projectile(Projectile *proj, ProjBlendMode *cur_blend_mode) {
	ProjBlendMode blend_mode;

	if(proj->flags & PFLAG_DRAWADD) {
		blend_mode = PBM_ADD;
	} else if(proj->flags & PFLAG_DRAWSUB) {
		blend_mode = PBM_SUB;
	} else {
		blend_mode = PBM_NORMAL;
	}

	if(blend_mode != *cur_blend_mode) {
		if(*cur_blend_mode == PBM_SUB) {
			glBlendEquation(GL_FUNC_ADD);
		}

		switch(blend_mode) {
			case PBM_NORMAL:
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				break;

			case PBM_ADD:
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				break;

			case PBM_SUB:
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
				break;
		}

		*cur_blend_mode = blend_mode;
	}

#ifdef PROJ_DEBUG
	if(proj->type == PlrProj) {
		set_debug_info(&proj->debug);
		log_fatal("Projectile with type PlrProj");
	}

	static Projectile prev_state;
	memcpy(&prev_state, proj, sizeof(Projectile));

	proj->draw_rule(proj, global.frames - proj->birthtime);

	if(memcmp(&prev_state, proj, sizeof(Projectile))) {
		set_debug_info(&proj->debug);
		log_fatal("Projectile modified its state in draw rule");
	}

	/*
	int cur_shader;
	glGetIntegerv(GL_CURRENT_PROGRAM, &cur_shader); // NOTE: this can be really slow!

	if(cur_shader != recolor_get_shader()->prog) {
		set_debug_info(&proj->debug);
		log_fatal("Bad shader after drawing projectile");
	}
	*/
#else
	proj->draw_rule(proj, global.frames - proj->birthtime);
#endif
}

void draw_projectiles(Projectile *projs, ProjPredicate predicate) {
	ProjBlendMode blend_mode = PBM_NORMAL;

	glUseProgram(recolor_get_shader()->prog);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if(predicate) {
		for(Projectile *proj = projs; proj; proj = proj->next) {
			if(predicate(proj)) {
				draw_projectile(proj, &blend_mode);
			}
		}
	} else {
		for(Projectile *proj = projs; proj; proj = proj->next) {
			draw_projectile(proj, &blend_mode);
		}
	}

	glUseProgram(0);

	if(blend_mode != PBM_NORMAL) {
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		if(blend_mode == PBM_SUB) {
			glBlendEquation(GL_FUNC_ADD);
		}
	}
}

bool projectile_in_viewport(Projectile *proj) {
	double w, h;
	int e = proj->max_viewport_dist;
	projectile_size(proj, &w, &h);

	return !(creal(proj->pos) + w/2 + e < 0 || creal(proj->pos) - w/2 - e > VIEWPORT_W
		  || cimag(proj->pos) + h/2 + e < 0 || cimag(proj->pos) - h/2 - e > VIEWPORT_H);
}

static Projectile* spawn_projectile_death_effect(Projectile *proj) {
	if(proj->tex == NULL) {
		return NULL;
	}

	return PARTICLE(
		.texture_ptr = proj->tex,
		.pos = proj->pos,
		.color = proj->color,
		.flags = proj->flags | PFLAG_NOREFLECT,
		.color_transform_rule = proj->color_transform_rule,
		.rule = timeout_linear,
		.draw_rule = DeathShrink,
		.args = { 10, 5*cexp(proj->angle*I) },
	);
}

Projectile* spawn_projectile_collision_effect(Projectile *proj) {
	if(proj->flags & PFLAG_NOCOLLISIONEFFECT) {
		return NULL;
	}

	return spawn_projectile_death_effect(proj);
}

Projectile* spawn_projectile_clear_effect(Projectile *proj) {
	if(proj->flags & PFLAG_NOCLEAREFFECT) {
		return NULL;
	}

	return spawn_projectile_death_effect(proj);
}

bool clear_projectile(Projectile **projlist, Projectile *proj, bool force, bool now) {
	if(proj->type >= PlrProj || (!force && !projectile_is_clearable(proj))) {
		return false;
	}

	if(now) {
		if(!(proj->flags & PFLAG_NOCLEARBONUS)) {
			create_bpoint(proj->pos);
		}

		spawn_projectile_clear_effect(proj);
		delete_projectile(projlist, proj);
	} else {
		proj->type = DeadProj;
	}

	return true;
}

void process_projectiles(Projectile **projs, bool collision) {
	ProjCollisionResult col = { 0 };

	char killed = 0;
	int action;

	for(Projectile *proj = *projs, *next; proj; proj = next) {
		next = proj->next;

		action = proj->rule(proj, global.frames - proj->birthtime);

		if(proj->type == DeadProj && killed < 5) {
			killed++;
			action = ACTION_DESTROY;

			if(clear_projectile(projs, proj, true, true)) {
				continue;
			}
		}

		if(collision) {
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

		if(action == ACTION_DESTROY) {
			col.fatal = true;
		}

		apply_projectile_collision(projs, proj, &col);
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
	if(p->type == DeadProj) {
		return true;
	}

	if(p->type == EnemyProj || p->type == FakeProj) {
		return (p->flags & PFLAG_NOCLEAR) != PFLAG_NOCLEAR;
	}

	return false;
}

int linear(Projectile *p, int t) { // sure is physics in here; a[0]: velocity
	if(t < 0)
		return 1;
	p->angle = carg(p->args[0]);
	p->pos = p->pos0 + p->args[0]*t;

	return 1;
}

int accelerated(Projectile *p, int t) {
	if(t < 0)
		return 1;
	p->angle = carg(p->args[0]);

	p->pos += p->args[0];
	p->args[0] += p->args[1];

	return 1;
}

int asymptotic(Projectile *p, int t) { // v = a[0]*(a[1] + 1); a[1] -> 0
	if(t < 0)
		return 1;
	p->angle = carg(p->args[0]);

	p->args[1] *= 0.8;
	p->pos += p->args[0]*(p->args[1] + 1);

	return 1;
}

static inline void apply_common_transforms(Projectile *proj, int t) {
	glTranslatef(creal(proj->pos), cimag(proj->pos), 0);
	glRotatef(proj->angle*180/M_PI+90, 0, 0, 1);

	if(t >= 16) {
		return;
	}

	if(proj->flags & PFLAG_NOSPAWNZOOM) {
		return;
	}

	if(proj->type != EnemyProj && proj->type != FakeProj) {
		return;
	}

	float s = 2.0-t/16.0;
	if(s != 1) {
		glScalef(s, s, 1);
	}
}

static inline void apply_color(Projectile *proj, Color c) {
	static ColorTransform ct;
	proj->color_transform_rule(proj, global.frames - proj->birthtime, c, &ct);
	recolor_apply_transform(&ct);
}

void ProjDrawCore(Projectile *proj, Color c) {
	apply_color(proj, c);
	draw_texture_p(0, 0, proj->tex);
}

void ProjDraw(Projectile *proj, int t) {
	glPushMatrix();
	apply_common_transforms(proj, t);
	ProjDrawCore(proj, proj->color);
	glPopMatrix();
}

void ProjNoDraw(Projectile *proj, int t) {
}

int blast_timeout(Projectile *p, int t) {
	if(t == 1) {
		p->args[1] = frand()*360 + frand()*I;
		p->args[2] = frand() + frand()*I;
	}

	return timeout(p, t);
}

void Blast(Projectile *p, int t) {
	glPushMatrix();
	glTranslatef(creal(p->pos), cimag(p->pos), 0);
	glRotatef(creal(p->args[1]), cimag(p->args[1]), creal(p->args[2]), cimag(p->args[2]));
	if(t != p->args[0] && p->args[0] != 0)
		glScalef(t/p->args[0], t/p->args[0], 1);

	apply_color(p, rgba(0.3, 0.6, 1.0, 1.0 - t/p->args[0]));

	draw_texture_p(0,0,p->tex);
	glScalef(0.5+creal(p->args[2]),0.5+creal(p->args[2]),1);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE);
	draw_texture_p(0,0,p->tex);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glPopMatrix();
}

void Shrink(Projectile *p, int t) {
	glPushMatrix();
	apply_common_transforms(p, t);

	float s = 2.0-t/p->args[0]*2;
	if(s != 1) {
		glScalef(s, s, 1);
	}

	ProjDrawCore(p, p->color);
	glPopMatrix();
}

void DeathShrink(Projectile *p, int t) {
	glPushMatrix();
	apply_common_transforms(p, t);

	float s = 2.0-t/p->args[0]*2;
	if(s != 1) {
		glScalef(s, 1, 1);
	}

	ProjDrawCore(p, p->color);
	glPopMatrix();
}

void GrowFade(Projectile *p, int t) {
	glPushMatrix();
	apply_common_transforms(p, t);

	float s = t/p->args[0]*(1 + (creal(p->args[2])? p->args[2] : p->args[1]));
	if(s != 1) {
		glScalef(s, s, 1);
	}

	ProjDrawCore(p, multiply_colors(p->color, rgba(1, 1, 1, 1 - t/p->args[0])));
	glPopMatrix();
}

void Fade(Projectile *p, int t) {
	glPushMatrix();
	apply_common_transforms(p, t);
	ProjDrawCore(p, multiply_colors(p->color, rgba(1, 1, 1, 1 - t/p->args[0])));
	glPopMatrix();
}

void ScaleFade(Projectile *p, int t) {
	glPushMatrix();
	apply_common_transforms(p, t);

	double scale_min = creal(p->args[2]);
	double scale_max = cimag(p->args[2]);
	double timefactor = t / creal(p->args[0]);
	double scale = scale_min * (1 - timefactor) + scale_max * timefactor;
	double alpha = pow(1 - timefactor, 2);

	// log_debug("%f %f %f %f", scale_min, scale_max, timefactor, scale);

	Color c = multiply_colors(p->color, rgba(1, 1, 1, alpha));

	glScalef(scale, scale, 1);
	ProjDrawCore(p, c);
	glPopMatrix();
}

int timeout(Projectile *p, int t) {
	if(t >= creal(p->args[0]))
		return ACTION_DESTROY;

	return 1;
}

int timeout_linear(Projectile *p, int t) {
	if(t >= creal(p->args[0]))
		return ACTION_DESTROY;
	if(t < 0)
		return 1;

	p->angle = carg(p->args[1]);
	p->pos = p->pos0 + p->args[1]*t;

	return 1;
}

int timeout_linear_fixangle(Projectile *p, int t) {
	if(t >= creal(p->args[0]))
		return ACTION_DESTROY;
	if(t < 0)
		return 1;

	p->pos = p->pos0 + p->args[1]*t;

	return 1;
}

void Petal(Projectile *p, int t) {
	float x = creal(p->args[2]);
	float y = cimag(p->args[2]);
	float z = creal(p->args[3]);

	float r = sqrt(x*x+y*y+z*z);
	x /= r; y /= r; z /= r;

	glDisable(GL_CULL_FACE);
	glPushMatrix();
	glTranslatef(creal(p->pos), cimag(p->pos),0);
	glRotatef(t*4.0 + cimag(p->args[3]), x, y, z);
	ProjDrawCore(p, p->color);
	glPopMatrix();
	glEnable(GL_CULL_FACE);
}

void petal_explosion(int n, complex pos) {
	for(int i = 0; i < n; i++) {
		tsrand_fill(6);
		float t = frand();
		Color c = rgba(sin(5*t),cos(5*t),0.5,t);

		PARTICLE("petal", pos, c, asymptotic,
			.draw_rule = Petal,
			.args = {
				(3+5*afrand(2))*cexp(I*M_PI*2*afrand(3)),
				5,
				afrand(4) + afrand(5)*I,
				afrand(1) + 360.0*I*afrand(0),
			},
			// hack: never reflect these in stage1 water (GL_CULL_FACE conflict)
			.flags = PFLAG_DRAWADD | PFLAG_NOREFLECT,
		);
	}
}

void projectiles_preload(void) {
	preload_resources(RES_TEXTURE, RESF_PERMANENT,
		// XXX: maybe split this up into stage-specific preloads too?
		// some of these are ubiquitous, but some only appear in very specific parts.

		"part/blast",
		"part/flare",
		"part/lasercurve",
		"part/petal",
		"part/smoke",
		"part/stain",
		"part/lightning0",
		"part/lightning1",
		"part/lightningball",
		"proj/ball",
		"proj/bigball",
		"proj/bullet",
		"proj/card",
		"proj/crystal",
		"proj/flea",
		"proj/hghost",
		"proj/plainball",
		"proj/rice",
		"proj/soul",
		"proj/thickrice",
		"proj/wave",
		"proj/youhoming",
	NULL);

	preload_resources(RES_SFX, RESF_PERMANENT,
		"shot1",
		"shot2",
		"shot1_loop",
		"shot_special1",
		"redirect",
	NULL);
}
