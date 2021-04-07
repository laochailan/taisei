/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_boss_h
#define IGUARD_boss_h

#include "taisei.h"

#include "util.h"
#include "difficulty.h"
#include "aniplayer.h"
#include "color.h"
#include "projectile.h"
#include "entity.h"
#include "coroutine.h"

#define BOSS_HURT_RADIUS 16
#define BOSS_MAX_ATTACKS 24

enum {
	ATTACK_START_DELAY = 60,
	ATTACK_START_DELAY_EXTRA = 150,
	ATTACK_END_DELAY = 20,
	ATTACK_END_DELAY_MOVE = 0,
	ATTACK_END_DELAY_SPELL = 60,
	ATTACK_END_DELAY_SURV = 20,
	ATTACK_END_DELAY_EXTRA = 150,
	ATTACK_END_DELAY_PRE_EXTRA = 60,
	BOSS_DEATH_DELAY = 120,
};

typedef struct Attack Attack;
typedef struct AttackInfo AttackInfo;

typedef void (*BossRule)(Boss*, int time) attr_nonnull(1);

typedef enum AttackType {
	AT_Normal,
	AT_Move,
	AT_Spellcard,
	AT_SurvivalSpell,
	AT_ExtraSpell,
	AT_Immediate,
} AttackType;

#define ATTACK_IS_SPELL(a) ((a) == AT_Spellcard || (a) == AT_SurvivalSpell || (a) == AT_ExtraSpell)

DEFINE_TASK_INTERFACE(BossAttack, {
	BoxedBoss boss;
	Attack *attack;
});

typedef TASK_INDIRECT_TYPE(BossAttack) BossAttackTask;
typedef TASK_IFACE_ARGS_TYPE(BossAttack) BossAttackTaskArgs;
typedef TASK_IFACE_ARGS_SIZED_PTR_TYPE(BossAttack) BossAttackTaskCustomArgs;

struct AttackInfo {
	/*
		HOW TO SET UP THE IDMAP FOR DUMMIES

		The idmap is used as a template to generate spellstage IDs in stage_init_array().
		It looks like this:

			{id_easy, id_normal, id_hard, id_lunatic}

		Where each of those IDs are in range of 0-127, and are unique within each stage-specific AttackInfo array.
		A special value of -1 can be used to not generate a spellstage for specific difficulties.
		This is useful if your spell is only available on Hard and Lunatic, or has a difficulty-dependent name, etc.

		IDs of AT_ExtraSpell attacks may overlap with those of other types.

		Most importantly DO NOT CHANGE ESTABLISHED IDs unless you absolutely must.
		Doing so is going to break replays, progress files, and anything that stores stage IDs permanently.
		Stage IDs are an internal detail invisible to the player, so they don't need to have any kind of fancy ordering.
	*/
	schar idmap[NUM_SELECTABLE_DIFFICULTIES + 1];

	AttackType type;
	char *name;
	float timeout;
	float hp;

	BossRule rule;
	BossRule draw_rule;

	cmplx pos_dest;
	int bonus_rank;

	// TODO: when we no longer need rule, this shall take its place
	TASK_INDIRECT_TYPE(BossAttack) task;
};

struct Attack {
	char *name;

	AttackType type;

	int starttime;
	int endtime;
	int endtime_undelayed;
	int failtime;
	int timeout;

	float bonus_base;
	float maxhp;
	float hp;

	BossRule rule;
	BossRule draw_rule;

	COEVENTS_ARRAY(
		initiated,   // before start delay
		started,     // after start delay
		finished,    // before end delay
		completed    // after end delay
	) events;

	AttackInfo *info; // NULL for attacks created directly through boss_add_attack
};

static inline bool attack_has_started(const Attack *atk) {
	return atk->events.started.num_signaled;
}

static inline bool attack_has_finished(const Attack *atk) {
	return atk->events.finished.num_signaled;
}

static inline bool attack_was_failed(const Attack *atk) {
	return atk->failtime > 0;
}

static inline bool attack_is_active(const Attack *atk) {
	return attack_has_started(atk) && !attack_has_finished(atk);
}

DEFINE_ENTITY_TYPE(Boss, {
	cmplx pos;

	Attack attacks[BOSS_MAX_ATTACKS];
	Attack *current;

	char *name;

	int acount;

	AniPlayer ani;
	Sprite portrait; // Used in spellcard intros

	Color zoomcolor;
	Color shadowcolor;
	Color glowcolor;

	int failed_spells;
	int lastdamageframe; // used to make the boss blink on damage taken
	int birthtime;

	BossRule global_rule;

	MoveParams move;

	// These are publicly accessible damage multipliers *you* can use to buff your spells.
	// Just change the numbers. global.shake_view style. 1.0 is the default.
	// If a new attack starts, they are reset. Nothing can go wrong!
	float bomb_damage_multiplier;
	float shot_damage_multiplier;

	struct {
		float opacity;
		float fill_total;
		float fill_alt;
		Color fill_color;
		Color fill_altcolor;
	} healthbar;

	struct {
		Attack *a_prev;
		Attack *a_cur;
		Attack *a_next;
		float global_opacity;
		float spell_opacity;
		float plrproximity_opacity;
		float attack_timer;
	} hud;

	COEVENTS_ARRAY(defeated) events;
});

Boss *create_boss(char *name, char *ani, cmplx pos) attr_nonnull(1, 2) attr_returns_nonnull;
void free_boss(Boss *boss) attr_nonnull(1);
void process_boss(Boss **boss) attr_nonnull(1);

void draw_extraspell_bg(Boss *boss, int time) attr_nonnull(1);
void draw_boss_background(Boss *boss) attr_nonnull(1);
void draw_boss_overlay(Boss *boss) attr_nonnull(1);
void draw_boss_fake_overlay(Boss *boss) attr_nonnull(1);

Attack *boss_add_attack(Boss *boss, AttackType type, char *name, float timeout, int hp, BossRule rule, BossRule draw_rule)
	attr_nonnull(1) attr_returns_nonnull;
Attack *boss_add_attack_task(Boss *boss, AttackType type, char *name, float timeout, int hp, BossAttackTask task, BossRule draw_rule)
	attr_nonnull(1) attr_returns_nonnull;
Attack *boss_add_attack_from_info(Boss *boss, AttackInfo *info, bool move)
	attr_nonnull(1, 2) attr_returns_nonnull;
Attack *boss_add_attack_from_info_with_args(Boss *boss, AttackInfo *info, BossAttackTaskCustomArgs args)
	attr_nonnull(1, 2) attr_returns_nonnull;
void boss_set_attack_bonus(Attack *a, int rank) attr_nonnull(1);

void boss_set_portrait(Boss *boss, const char *name, const char *variant, const char *face) attr_nonnull(1);

void boss_engage(Boss *b) attr_nonnull(1);
void boss_start_next_attack(Boss *b, Attack *a) attr_nonnull(1, 2);
void boss_finish_current_attack(Boss *boss) attr_nonnull(1);

bool boss_is_dying(Boss *boss) attr_nonnull(1); // true if the last attack is over but the BOSS_DEATH_DELAY has not elapsed.
bool boss_is_fleeing(Boss *boss) attr_nonnull(1);
bool boss_is_vulnerable(Boss *boss) attr_nonnull(1);
bool boss_is_player_collision_active(Boss *boss) attr_nonnull(1);

void boss_death(Boss **boss) attr_nonnull(1);

void boss_reset_motion(Boss *boss) attr_nonnull(1);

void boss_preload(void);

#define BOSS_DEFAULT_SPAWN_POS (VIEWPORT_W * 0.5 - I * VIEWPORT_H * 0.5)
#define BOSS_DEFAULT_GO_POS (VIEWPORT_W * 0.5 + 200.0*I)
#define BOSS_NOMOVE (-3142-39942.0*I)

Boss *_init_boss_attack_task(const BossAttackTaskArgs *args)
	attr_nonnull_all attr_returns_nonnull;
void _begin_boss_attack_task(const BossAttackTaskArgs *args)
	attr_nonnull_all;
#define INIT_BOSS_ATTACK(_args) _init_boss_attack_task(_args)
#define BEGIN_BOSS_ATTACK(_args) _begin_boss_attack_task(_args)

int attacktype_start_delay(AttackType t) attr_const;
int attacktype_end_delay(AttackType t) attr_const;

#endif // IGUARD_boss_h
