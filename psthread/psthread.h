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

/*! \file psthread.h
    \brief PSthreads application programming interface
 */

#ifndef __PS_THREAD_H__
#define __PS_THREAD_H__

#define PSTHREAD_VERSION	(104)
 
#include "locks.h"


/*****************************************
 * Basic types definitions
 */

typedef void *psthread_t;
typedef int psthread_key_t;

/*****************************************
 * Thread attributes
 */

#define PSTHREAD_JOINABLE	0
#define PSTHREAD_DETACHED	1
#define PSTHREAD_STATIC		2

#define PSTHREAD_ATTR_INITIALIZER	{PSTHREAD_JOINABLE, NULL}

typedef struct psthread_attr_tag {
	int detachstate;
	psthread_t parent;
} psthread_attr_t;


/*****************************************
 * Synchronization
 */

typedef _lock_t psthread_lock_t;
 
/******************************************
 * Condition variable (queue based implementation)
 */

typedef union psthread_cond_tag {	/* or struct without q */
	struct {
		psthread_lock_t lock;
		psthread_t head;
		psthread_t tail;
	} q;
	char pad[CACHE_LINE_SIZE];
} psthread_cond_t;

/*****************************************
 * Structure describing a barrier (D. Butenhof)
 */


typedef struct psthread_barrier_tag {
	/* char size[SIZE_OF_BARRIER_T]; */
	psthread_cond_t cv;		/* wait for barrier */
	psthread_lock_t mutex;		/* Control access to barrier */
	long valid;			/* set when valid */
	long threshold;			/* number of threads required */
	long counter;			/* current number of threads */
	unsigned long cycle;		/* count cycles */
//	void *aux;			/* auxiliary space */
//	void (*ppf)(void *, int);       /* auxiliary function */        
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
	/* char size[SIZE_OF_BARRIER_T]; */
//	psthread_cond_t cv;		/* wait for barrier */
	psthread_lock_t mutex;		/* Control access to barrier */
	long valid;			/* set when valid */
	long threshold;			/* number of threads required */
	long counter;			/* current number of threads */
//	unsigned long cycle;		/* count cycles */
	void *aux;			/* auxiliary space */
        void (*ppf)(void *, int);       /* auxiliary function */        
	psthread_t *threads;		/* ... */

	/* yield-based barrier */
	ps_aligned_int *arrived;
	int sense, nthr;
} psthread_barrier_ex_t;

/*****************************************
 * Interface
 */

int psthread_init(int vps, unsigned long stacksize);

int psthread_create (psthread_t *t, psthread_attr_t *att, void (*start)(void *arg), void *arg);
int psthread_create_ex (psthread_t *t, psthread_attr_t *att, void (*start)(void *arg), void *arg);
int psthread_waitall();

int psthread_enqueue(psthread_t thread, int queue);
int psthread_enqueue_head(psthread_t thread, int queue);
int psthread_enqueue_tail(psthread_t thread, int queue);
int psthread_enqueue_bunch_tail(psthread_t *thread, int nthreads, int queue);

psthread_t psthread_self(void);
psthread_t psthread_self_ex(int vp);
psthread_t psthread_self_stk(void);
void psthread_exit();
void psthread_yield();
void psthread_sleep(int ms);
void psthread_switch_and_yield(psthread_t from, psthread_t to);

void psthread_lock_init(psthread_lock_t *spin);
int psthread_lock_acquire(psthread_lock_t *spin);
int psthread_lock_try_acquire(psthread_lock_t *spin);
int psthread_lock_release(psthread_lock_t *spin);
void psthread_lock_destroy(psthread_lock_t *spin);

void psthread_cond_init(psthread_cond_t *cond);
void psthread_cond_wait (psthread_cond_t *cond, psthread_lock_t *lock);
void psthread_cond_wait_ex (psthread_t thread, psthread_cond_t *cond, psthread_lock_t *lock);
void psthread_cond_signal (psthread_cond_t *cond);
void psthread_cond_signal_ex (psthread_cond_t *cond);
void psthread_cond_broadcast (psthread_cond_t *cond);
void psthread_cond_destroy (psthread_cond_t *cond);

int psthread_barrier_init (psthread_barrier_t *barrier, int count);
int psthread_barrier_destroy (psthread_barrier_t *barrier);
int psthread_barrier_wait (psthread_barrier_t *barrier);

int psthread_barrier_init_ex (psthread_barrier_ex_t *barrier, int count);
int psthread_barrier_destroy_ex (psthread_barrier_ex_t *barrier);
int psthread_barrier_wait_ex (psthread_barrier_ex_t *barrier, int id);

double psthread_gettime();

int psthread_key_create(psthread_key_t *key, void *destructor);
void *psthread_getspecific(psthread_key_t key);
int psthread_setspecific(psthread_key_t key, const void *value);
int psthread_key_clone(psthread_t thread1, psthread_t thread2);
void *psthread_getspecific_ex(psthread_t thr, psthread_key_t key);
int psthread_setspecific_ex(psthread_t thr, psthread_key_t key, const void *value);

int psthread_nvps(void);
int psthread_current_vp(void);
unsigned long psthread_vp_self(void);
void psthread_vp_sleep(int ms);
int psthread_stealing(int status);

void psthread_version(void);
void psthread_stats(void);
#endif /* !__PS_THREAD_H__ */



