/* 
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is the Netscape Portable Runtime library.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 1994-2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):  Silicon Graphics, Inc.
 * 
 * Portions created by SGI are Copyright (C) 2000-2001 Silicon
 * Graphics, Inc.  All Rights Reserved.
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */

 /* P. E. Hadjidoukas: This file has been derived from the 
  * original common.h file of the State Threads Library. It is a core
  * version of it, extended appropriately for our PSthreads/UthLib package. 
  * For more information, you can read the documentation file */
	
#ifndef __UTH_COMMON_H__
#define __UTH_COMMON_H__

#include "config.h"
#include "uth_opt.h"

#include "locks.h"
#include "queues.h"
#include "alloc.h"

#include <stddef.h>
#include <sys/types.h>
#include <pthread.h>





/* Enable assertions only if DEBUG is defined */
#define ST_BEGIN_MACRO  {
#define ST_END_MACRO    }

#if defined (UTH_CTXSW_SJLJ)
#include <setjmp.h>
#include "md.h"
#elif defined(UTH_CTXSW_MCSC)
#if defined(CYGWIN) || defined(MINGW)
#include <win32_ucontext.h>
#else
#include <ucontext.h>
#endif
#if UTH_STACK_GROWS < 0
#define MD_STACK_GROWS_DOWN
#else
#define MD_STACK_GROWS_UP
#endif
#define MD_STACK_PAD_SIZE 64
#else
#error "unknown context switch method"
#endif


/*****************************************
 * Basic types definitions
 */

typedef struct _uth_stack {
  char *raddr;                /* Real address of allocated memory */
  char *vaddr;                /* Base of stack's allocated memory */
  int  vaddr_size;            /* Size of stack's allocated memory */
  int  stk_size;              /* Size of usable portion of the stack */
  char *stk_bottom;           /* Lowest address of stack's usable portion */
  char *stk_top;              /* Highest address of stack's usable portion */
  void *sp;                   /* Stack pointer from C's point of view */
#ifdef __ia64__
  void *bsp;                  /* Register stack backing store pointer */
#endif
} uth_stack_t;

typedef struct _uth_thread {
  void (*start)(void *arg);	/* The start function of the thread */
  void *arg;			/* Argument of the start function */
  void *retval;			/* Return value of the start function - Used as auxiliary space */
  unsigned long vp_id;		/* On wich virtual processor thread runs */
  unsigned long vp_cr;

  QUEUE_NODE_FIELDS(struct _uth_thread);
/*  struct _uth_thread *prev;*/
/*  struct _uth_thread *next;*/

  uth_stack_t *stack;		/* Info about thread's stack */
  uth_stack_t stackinfo;





#if defined(UTH_CTXSW_SJLJ)	            /* Thread's context */
  jmp_buf context;
#elif defined(UTH_CTXSW_MCSC)
  ucontext_t context;
#else
#error "unknown context switch method"
#endif
} uth_t;


/* Data structure */
QUEUE_DEFINE(uth_t,queue_t);	/* queue_t for uth_t structures */ 


/* Internal library global data */
typedef struct uth_data {	/* padding required */
        queue_t			reuse_uth_q1;
        queue_t		        reuse_uth_lq1[MAX_VPS];

	uth_t			*_uth_this_thread[MAX_VPS][CACHE_LINE_SIZE/SIZEOF_VOID_P];	/* current thread */

	/* read only */
	pthread_t 		vp_ids1[MAX_VPS][CACHE_LINE_SIZE/SIZEOF_PTHREAD_T];		/* kernel thread identifiers */

	int			pagesize;	/* platform specific */
	int			stacksize;	/* fixed stack size for threads */
} uth_data_t;


typedef struct uth_stats {     /* padding required */
        unsigned long		_stacks_created[MAX_VPS][CACHE_LINE_SIZE/SIZEOF_LONG];
        unsigned long		_stacks_reused[MAX_VPS][CACHE_LINE_SIZE/SIZEOF_LONG];
        unsigned long		_stacks_queued[MAX_VPS][CACHE_LINE_SIZE/SIZEOF_LONG];
} uth_stats_t;


/*****************************************
 * Macro definitions
 */


extern uth_stats_t                              _uth_stats;

#define _UTH_CURRENT_THREAD(_vp)		(_uth_data->_uth_this_thread[_vp][0])
#if defined(UTH_SI_KEEP_CURRENT_THREAD)
#define _UTH_SET_CURRENT_THREAD(_vp, _thread)	(_uth_data->_uth_this_thread[_vp][0] = (_thread))
#else
#define _UTH_SET_CURRENT_THREAD(_vp, _thread)
#endif

#define _UTH_PAGE_SIZE				(_uth_data->pagesize)
#define _UTH_STACK_SIZE				(_uth_data->stacksize)

#define	reuse_uth_q				(_uth_data->reuse_uth_q1)
#define	reuse_uth_lq				(_uth_data->reuse_uth_lq1)
#define vp_ids					(_uth_data->vp_ids1)

#define stacks_created(_vp)			(_uth_stats._stacks_created[_vp][0])
#define stacks_reused(_vp)			(_uth_stats._stacks_reused[_vp][0])
#define stacks_queued(_vp)			(_uth_stats._stacks_queued[_vp][0])

extern uth_data_t                               *_uth_data;

#define uth_queue_empty(q) 			((q)->first==NULL)


/*****************************************
 * Threads context switching
 */

#if defined(UTH_CTXSW_SJLJ)
/*
 * Switch away from the current thread context by saving its state and
 * calling the thread scheduler
 */

#define _UTH_SWITCH_CONTEXT(_vp, _thread, _next)	\
	ST_BEGIN_MACRO					\
	if (!MD_SETJMP((_thread)->context)) {		\
		_UTH_RESTORE_CONTEXT(_vp, _next);	\
	}						\
	ST_END_MACRO

/*
 * Restore a thread context that was saved by _UTH_SWITCH_CONTEXT or
 * initialized by _UTH_INIT_CONTEXT
 */

#define _UTH_RESTORE_CONTEXT(_vp, _thread)		\
	ST_BEGIN_MACRO					\
	_UTH_SET_CURRENT_THREAD(_vp, _thread);		\
	MD_LONGJMP((_thread)->context, 1);		\
	ST_END_MACRO

#elif defined (UTH_CTXSW_MCSC)
/*
 * Switch away from the current thread context to the next
 * thread context
 */

#if defined(SOLARIS) && defined(i386)
#define _UTH_SWITCH_CONTEXT(_vp, _thread, _next)		\
	ST_BEGIN_MACRO									\
	_UTH_SET_CURRENT_THREAD(_vp, _next);			\
	getcontext(&(_thread)->context); \
	swapcontext(&(_thread)->context,&(_next)->context); \
	ST_END_MACRO
#else
#define _UTH_SWITCH_CONTEXT(_vp, _thread, _next)		\
	ST_BEGIN_MACRO						\
	_UTH_SET_CURRENT_THREAD(_vp, _next);			\
	swapcontext(&(_thread)->context,&(_next)->context); \
	ST_END_MACRO
#endif

/*
 * Restore a thread context that was saved by _UTH_SWITCH_CONTEXT or
 * initialized by _UTH_INIT_CONTEXT
 */
#if defined(SOLARIS) && defined(i386)
#define _UTH_RESTORE_CONTEXT(_vp, _thread)	\
	ST_BEGIN_MACRO				\
	uth_t *temp;				\
	temp = _UTH_CURRENT_THREAD(_vp);	\
	_UTH_SET_CURRENT_THREAD(_vp, _thread);	\
	getcontext(&temp->context);		\
	swapcontext(&temp->context, &(_thread)->context);		\
	ST_END_MACRO
#else
#define _UTH_RESTORE_CONTEXT(_vp, _thread)	\
	ST_BEGIN_MACRO				\
	_UTH_SET_CURRENT_THREAD(_vp, _thread);	\
	setcontext(&(_thread)->context);	\
	ST_END_MACRO
#endif

#else
#error "unknown context switch method"
#endif

/*
 * Initialize the thread context preparing it to execute _main
 */
#if defined(UTH_CTXSW_SJLJ)

#ifdef MD_INIT_CONTEXT
#define _UTH_INIT_CONTEXT MD_INIT_CONTEXT
#else
#error Unknown OS
#endif

#elif defined(UTH_CTXSW_MCSC)

#define _UTH_INIT_CONTEXT(_thread, _sp, _main)		\
  ST_BEGIN_MACRO                                        \
  getcontext(&(_thread)->context);			\
  (_thread)->context.uc_link = NULL;			\
  (_thread)->context.uc_stack.ss_size = UTH_MCSC_SKSIZE((char *)(_thread)->stack->stk_bottom, (char *)_sp-(char *)(_thread)->stack->stk_bottom); \
  (_thread)->context.uc_stack.ss_sp = UTH_MCSC_SKADDR((char *)(_thread)->stack->stk_bottom, (char *)_sp-(char *)(_thread)->stack->stk_bottom); \
  (_thread)->context.uc_stack.ss_flags = 0;		\
  makecontext(&(_thread)->context,_main,0);		\
  ST_END_MACRO


#else
#error "unknown context switch method"
#endif

/*
 * Number of bytes reserved under the stack "bottom"
 */
#define _UTH_STACK_PAD_SIZE MD_STACK_PAD_SIZE


/*****************************************
 * Forward declarations
 */

void _uth_main(void);
void uth_reinit (uth_t *thread, void (*start)(void *arg), void *arg);
int uth_get_vpid(void);
uth_stack_t *_uth_stack_new(uth_stack_t *stackinfo, int stack_size);
uth_stack_t *_uth_stack_new_sp(uth_stack_t *stackinfo, int stack_size, char *vaddr);
void _uth_stack_free(uth_stack_t *ts);
uth_t *_uth_self_from_stack(void);
int _uth_get_vpid_tsd(void);
int uth_get_vpid_tsd(void);

#endif /* !__UTH_COMMON_H__ */
