/*
  PSthreads and UthLib thread libraries
  == Copyright (C) 2007 the OMPi/PSthreads Team
  == Department of Computer Science, University of Ioannina

  This file is part of PSthreads.

  PSthreads is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  PSthreads is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with PSthreads; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*! \file common.h
    \brief Internal PSthreads definitions
 */

#ifndef __PS_COMMON_H__
#define __PS_COMMON_H__

#include "config.h"
#include "locks.h"
#include "queues.h"
#include "alloc.h"
#include <uth.h>

/*****************************************
 * Syncronization
 */

typedef _lock_t psthread_lock_t;
 
void psthread_lock_init(psthread_lock_t *spin);
int psthread_lock_acquire(psthread_lock_t *spin);
int psthread_lock_try_acquire(psthread_lock_t *spin);
int psthread_lock_release(psthread_lock_t *spin);
void psthread_lock_destroy(psthread_lock_t *spin);

/******************************************
 * Condition variable (queue based implementation)
 */

/* Data structure */
QUEUE_DEFINE(struct _psthread, queue_t);    /* queue_t for psthread_t structures */

QUEUE_DEFINE(struct _psthread, psthread_cond_t);    /* queue_t for psthread_cond_t structures */


/*****************************************
 * Structure describing a barrier (D. Butenhof)
 */

#define _PS_BARRIER_VALID   0xdbcafe

typedef struct psthread_barrier_tag {
	psthread_cond_t cv;		/* wait for barrier */
	psthread_lock_t mutex;		/* Control access to barrier */
	long valid;			/* set when valid */
	long threshold;			/* number of threads required */
	long counter;			/* current number of threads */
	unsigned long cycle;		/* count cycles */
//	void *aux;			/* auxiliary space */
//	void (*ppf)(void *, int);	/* auxiliary function */
} psthread_barrier_t;


/*****************************************
 * Structure describing a fast barrier
 */

/* This is a padded int type so as to occupy a whole cache line.
 * We use it mainly for per-thread flags so that there is no
 * contention among the threads (adopted from OMPi).
 */
typedef union {
        int  value;
        char padding[CACHE_LINE_SIZE];         
} ps_aligned_int;


typedef struct psthread_barrier_ex_tag {
//	psthread_cond_t cv;		/* wait for barrier */
	psthread_lock_t mutex;		/* Control access to barrier */
	long valid;			/* set when valid */
	long threshold;			/* number of threads required */
	long counter;			/* current number of threads */
//	unsigned long cycle;		/* count cycles */
	void *aux;			/* auxiliary space */
	void (*ppf)(void *, int);	/* auxiliary function */
	struct _psthread **threads;

	/* yield-based barrier */
        ps_aligned_int *arrived;
        int sense, nthr;
} psthread_barrier_ex_t;

/*****************************************
 * Basic types definitions
 */
 
#define MAX_TLS_KEYS	16

typedef struct _psthread {
	int state;			/* Thread's state */
	int detachstate;		/* 0: joinable (waitall), 1: detached */
	int staticmem;
	struct _psthread *cleanup;

	void (*start)(void *arg);	/* The start function of the thread */
	void *arg;			/* Argument of the start function */
	
	psthread_lock_t lock;
	
	uth_t context;            	/* Thread's context */
	struct _psthread *prev;	  	/* previous thread in the ready queue or zombie queue */
	struct _psthread *next;    	/* next thread in the ready queue or zombie queue */

	int nchildren;			/* Active children created by this thread */
	struct _psthread *parent;	/* Thread's parent */
	struct _psthread *toq;		/* Thread's parent */

	int vpid;			/* vp the thread runs on */

	const void *tls[MAX_TLS_KEYS];	/* thread local storage */
} psthread_t;


#define PSTHREAD_JOINABLE       	0
#define PSTHREAD_DETACHED       	1

#define PSTHREAD_ATTR_INITIALIZER	{0, NULL}

typedef struct psthread_attr_tag {        
	int detachstate;
	struct _psthread *parent;
} psthread_attr_t;


typedef int psthread_key_t;

/*****************************************
 * Current vp and thread
 */

#ifndef MAX_VPS
#define MAX_VPS	16
#endif
extern psthread_t	*_ps_scheduler_thread[MAX_VPS];   /* Scheduler thread for this vp */

/*****************************************
 * Thread states and flags
 */

#define _PS_NEW			0
#define _PS_RUNNING		1
#define _PS_RUNNABLE		2
#define _PS_ZOMBIE		3
#define _PS_WAITING		4
#define _PS_FREED		5

/*****************************************
 * Constants
 */

#define _PS_DEFAULT_STACK_SIZE (4*1024*1024)


#endif /* !__PS_COMMON_H__ */

