/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_coroutine_h
#define IGUARD_coroutine_h

#include "taisei.h"

#include "entity.h"
#include <koishi.h>

typedef struct CoTask CoTask;
typedef struct CoSched CoSched;
typedef void *(*CoTaskFunc)(void *arg);

// target for the INVOKE_TASK macro
extern CoSched *_cosched_global;

typedef enum CoStatus {
	CO_STATUS_SUSPENDED = KOISHI_SUSPENDED,
	CO_STATUS_RUNNING   = KOISHI_RUNNING,
	CO_STATUS_DEAD      = KOISHI_DEAD,
} CoStatus;

typedef struct CoEvent {
	ListAnchor subscribers;
	uint32_t unique_id;
	uint32_t num_signaled;
} CoEvent;

void coroutines_init(void);
void coroutines_shutdown(void);

CoTask *cotask_new(CoTaskFunc func);
void cotask_free(CoTask *task);
void *cotask_resume(CoTask *task, void *arg);
void *cotask_yield(void *arg);
bool cotask_wait_event(CoEvent *evt, void *arg);
CoStatus cotask_status(CoTask *task);
CoTask *cotask_active(void);
void cotask_bind_to_entity(CoTask *task, EntityInterface *ent);
void cotask_set_finalizer(CoTask *task, CoTaskFunc finalizer, void *arg);

void coevent_init(CoEvent *evt);
void coevent_signal(CoEvent *evt);
void coevent_signal_once(CoEvent *evt);
void coevent_cancel(CoEvent *evt);

CoSched *cosched_new(void);
void *cosched_new_task(CoSched *sched, CoTaskFunc func, void *arg);  // creates and runs the task, returns whatever it yields, schedules it for resume on cosched_run_tasks if it's still alive
uint cosched_run_tasks(CoSched *sched);  // returns number of tasks ran
void cosched_free(CoSched *sched);

static inline attr_must_inline void cosched_set_invoke_target(CoSched *sched) { _cosched_global = sched; }

#define TASK_ARGS_NAME(name) COARGS_##name
#define TASK_ARGS(name) struct TASK_ARGS_NAME(name)

#define TASK_ARGSCOND_NAME(name) COARGSCOND_##name
#define TASK_ARGSCOND(name) struct TASK_ARGSCOND_NAME(name)

#define ARGS (*_cotask_args)

#define TASK_COMMON_DECLARATIONS(name, argstruct) \
	/* produce warning if the task is never used */ \
	static char COTASK_UNUSED_CHECK_##name; \
	/* user-defined type of args struct */ \
	struct TASK_ARGS_NAME(name) argstruct; \
	/* type of internal args struct for INVOKE_TASK_WHEN */ \
	struct TASK_ARGSCOND_NAME(name) { TASK_ARGS(name) real_args; CoEvent *event; }; \
	/* user-defined task body */ \
	static void COTASK_##name(TASK_ARGS(name) *_cotask_args); \
	/* called from the entry points before task body (inlined, hopefully) */ \
	static inline attr_must_inline void COTASKPROLOGUE_##name(TASK_ARGS(name) *_cotask_args); \
	/* task entry point for INVOKE_TASK */ \
	attr_unused static void *COTASKTHUNK_##name(void *arg) { \
		/* copy args to our coroutine stack so that they're valid after caller returns */ \
		TASK_ARGS(name) args_copy = *(TASK_ARGS(name)*)arg; \
		/* call prologue */ \
		COTASKPROLOGUE_##name(&args_copy); \
		/* call body */ \
		COTASK_##name(&args_copy); \
		/* exit coroutine */ \
		return NULL; \
	} \
	/* task entry point for INVOKE_TASK_WHEN */ \
	attr_unused static void *COTASKTHUNKCOND_##name(void *arg) { \
		/* copy args to our coroutine stack so that they're valid after caller returns */ \
		TASK_ARGSCOND(name) args_copy = *(TASK_ARGSCOND(name)*)arg; \
		/* wait for event, and if it wasn't canceled... */ \
		if(WAIT_EVENT(args_copy.event)) { \
			/* call prologue */ \
			COTASKPROLOGUE_##name(&args_copy.real_args); \
			/* call body */ \
			COTASK_##name(&args_copy.real_args); \
		} \
		/* exit coroutine */ \
		return NULL; \
	}

/* define a normal task */
#define TASK(name, argstruct) \
	TASK_COMMON_DECLARATIONS(name, argstruct) \
	/* empty prologue */ \
	static inline attr_must_inline void COTASKPROLOGUE_##name(TASK_ARGS(name) *_cotask_args) { } \
	/* begin task body definition */ \
	static void COTASK_##name(TASK_ARGS(name) *_cotask_args)

/* define a task that needs a finalizer */
#define TASK_WITH_FINALIZER(name, argstruct) \
	TASK_COMMON_DECLARATIONS(name, argstruct) \
	/* error out if using TASK_FINALIZER without TASK_WITH_FINALIZER */ \
	struct COTASK__##name##__not_declared_using_TASK_WITH_FINALIZER { char dummy; }; \
	/* user-defined finalizer function */ \
	static inline attr_must_inline void COTASKFINALIZER_##name(TASK_ARGS(name) *_cotask_args); \
	/* real finalizer entry point */ \
	static void *COTASKFINALIZERTHUNK_##name(void *arg) { \
		COTASKFINALIZER_##name((TASK_ARGS(name)*)arg); \
		return NULL; \
	} \
	/* prologue; sets up finalizer before executing task body */ \
	static inline attr_must_inline void COTASKPROLOGUE_##name(TASK_ARGS(name) *_cotask_args) { \
		cotask_set_finalizer(cotask_active(), COTASKFINALIZERTHUNK_##name, _cotask_args); \
	} \
	/* begin task body definition */ \
	static void COTASK_##name(TASK_ARGS(name) *_cotask_args)

/* define the finalizer for a TASK_WITH_FINALIZER */
#define TASK_FINALIZER(name) \
	/* error out if using TASK_FINALIZER without TASK_WITH_FINALIZER */ \
	attr_unused struct COTASK__##name##__not_declared_using_TASK_WITH_FINALIZER COTASK__##name##__not_declared_using_TASK_WITH_FINALIZER; \
	/* begin finalizer body definition */ \
	static void COTASKFINALIZER_##name(TASK_ARGS(name) *_cotask_args)

#define INVOKE_TASK(name, ...) do { \
	(void)COTASK_UNUSED_CHECK_##name; \
	TASK_ARGS(name) _coargs = { __VA_ARGS__ }; \
	cosched_new_task(_cosched_global, COTASKTHUNK_##name, &_coargs); \
} while(0)

#define INVOKE_TASK_WHEN(_event, name, ...) do { \
	(void)COTASK_UNUSED_CHECK_##name; \
	TASK_ARGSCOND(name) _coargs = { .real_args = { __VA_ARGS__ }, .event = (_event) }; \
	cosched_new_task(_cosched_global, COTASKTHUNKCOND_##name, &_coargs); \
} while(0)

#define YIELD         cotask_yield(NULL)
#define WAIT(delay)   do { int _delay = (delay); while(_delay-- > 0) YIELD; } while(0)
#define WAIT_EVENT(e) cotask_wait_event((e), NULL)

// to use these inside a coroutine, define a BREAK_CONDITION macro and a BREAK label.
#define CHECK_BREAK   do { if(BREAK_CONDITION) goto BREAK; } while(0)
#define BYIELD        do { YIELD; CHECK_BREAK; } while(0)
#define BWAIT(frames) do { WAIT(frames); CHECK_BREAK; } while(0)

#define TASK_BIND(ent) cotask_bind_to_entity(cotask_active(), &ent->entity_interface)

#endif // IGUARD_coroutine_h
