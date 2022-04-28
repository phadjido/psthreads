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

/*! \file sched_pth.c
    \brief Emulation of user-level threads
 */
 
#include "config.h"
#include "uth_opt.h"
#include "locks.h"
#include "queues.h"
#include "alloc.h"
#include <stdio.h>
#include <stdlib.h>
#if !defined(WIN32_MSVC) && !defined(MINGW)
#include <unistd.h>
#include <fcntl.h>
#else
/*#define _WIN32_WINNT	0x0500*/
#include <windows.h>
#endif
#include <string.h>

typedef struct _uth_t {
        void            (*func)(void *arg);
        void            *arg;
        int		pth_created;
        pthread_t       pth_handler;
        pthread_cond_t  pth_cond;
        pthread_mutex_t pth_mutex;
        void		*mem;
        int             vp_id;
        int             stacksize;
        struct _uth_t   *prev;
        struct _uth_t   *next;
} * uth_t;

/* Forward declarations */
uth_t uth_create(void (*start)(void *arg), void *arg);

QUEUE_DEFINE(struct _uth_t, queue_t);

queue_t rsq;
queue_t lrsq[MAX_VPS];

void _uth_to_rsq_end(struct _uth_t *uth)
{
	_enqueue_head(&rsq, uth);
}
	
struct _uth_t *_uth_reuseq_dequeue ()
{
	struct _uth_t *uth = NULL;

	_dequeue(&rsq, &uth);
	return uth;
}

void _uth_to_reuselrq(int queue, struct _uth_t *uth)
{
	_enqueue_head(&lrsq[queue], uth);
}
	
struct _uth_t *_uth_reuselrq_dequeue(int queue)
{
	struct _uth_t *uth = NULL;

	_dequeue(&lrsq[queue], &uth);
	return uth;
}

struct _uth_t *_uth_get_reused_thread(void)
{
	struct _uth_t *thread;

#if defined(UTH_LOCAL_REUSE)
	int vp = uth_get_vpid();
	thread = _uth_reuselrq_dequeue (vp);
#else          
	thread = _uth_reuseq_dequeue();
#endif
	return thread;
}

/* General routine: recycles a finished thread */
void _uth_put_reused_thread(struct _uth_t *thread)
{
#if defined(UTH_LOCAL_REUSE)
	int vp = uth_get_vpid();
/*	int vp = thread->vp_id;*/

	_uth_to_reuselrq(vp, thread);
#else
	_uth_to_reuseq(thread);
#endif
}

/* Key for the thread-specific buffer */
static pthread_key_t _uth_key;
/* Once-only initialisation of the key */
static pthread_once_t _uth_key_once = PTHREAD_ONCE_INIT;

/* Free the thread-specific buffer */
static void _uth_destroy(void * buf)
{
        free(buf);
}

/* Allocate the key */
static void _uth_key_alloc(void)
{
        pthread_key_create(&_uth_key, _uth_destroy);
}

/* Allocate the thread-specific buffer */
void _uth_alloc(void)
{
        pthread_once(&_uth_key_once, _uth_key_alloc);
        pthread_setspecific(_uth_key, malloc(sizeof(void *)));
}


/* Return the thread-specific buffer */
void *_uth_get_self(void)
{
	void **ThisFib;

	ThisFib = (void **) pthread_getspecific(_uth_key);
	return (*ThisFib);
}


void _uth_set_self(void *uth)
{
	void **this_uth;

	this_uth = (void **) pthread_getspecific(_uth_key);
	*this_uth = uth;
}

/* Allocate the thread-specific buffer */
void _uth_init_self(void *uth)
{
	void **this_uth;

	_uth_alloc();
	this_uth = (void **) pthread_getspecific(_uth_key);
	*this_uth = uth;
}


uth_t uth_self()
{
	struct _uth_t *uth = (struct _uth_t *) _uth_get_self();

	/*printf("uth_self: %p\n", uth);*/
	return uth;
}

uth_t uth_self_ex(int vpid)
{
	return uth_self();
}

int uth_get_vpid()
{
	return uth_self()->vp_id;
}


void *uth_getarg(uth_t thread)
{
	return thread->arg;
}


void uth_setarg(uth_t thread, void *arg)
{
        thread->arg = arg;
}


void *uth_getarg_ex(int vp)
{
	return uth_self()->arg;
}

pthread_attr_t attr;

int uth_init(int stacksize)
{
	int i;

	pthread_attr_init(&attr);

	pthread_attr_setstacksize(&attr, stacksize);
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	
	_queue_init(&rsq);
	for (i = 0; i < MAX_VPS; i++) _queue_init(&lrsq[i]);
	return 0;
}

int uth_vp_init(int vp, void *arg)
{
	struct _uth_t *uth /* = uth_create(0, arg)*/;

	uth = _ort_calloc_aligned(sizeof(struct _uth_t), NULL);
	pthread_cond_init(&uth->pth_cond, NULL);
	pthread_mutex_init (&uth->pth_mutex, NULL);
	uth->func = NULL;
	uth->arg = arg;
	uth->prev = uth->next = NULL;
	uth->pth_created = 1;
	uth->pth_handler = pthread_self();
	uth->vp_id = vp;
	_uth_init_self(uth);	/* in thread local storage */

	return 0;
}


uth_t uth_create(void (*start)(void *arg), void *arg)
{
	struct _uth_t *uth = NULL;

#if defined(UTH_REUSE)
	uth = _uth_get_reused_thread();
#endif
	if (uth == NULL) {
		void *actual;
		/*stksize = 8*1024*1024;*/
		uth = _ort_calloc_aligned(sizeof(struct _uth_t), &actual);	/* even recycle */
		uth->mem = actual;
		pthread_cond_init(&uth->pth_cond, NULL);
		pthread_mutex_init (&uth->pth_mutex, NULL);
		uth->pth_created = 0;
	}
	uth->func = start;
	uth->arg = arg;
	uth->prev = uth->next = NULL;

	return uth;
}


void uth_delete(uth_t thread)
{
	struct _uth_t *uth = thread;

#if defined(UTH_REUSE)
	_uth_put_reused_thread(uth);
#else
	pthread_cond_destroy(&uth->pth_cond);
	pthread_mutex_destroy (&uth->pth_mutex);
	/*uth->pth_handler = (pthread_t)NULL;*/
	free(uth->mem);
#endif
}

void *_uth_main(void *arg)
{
	struct _uth_t *uth = (struct _uth_t *) arg;
	_uth_init_self(uth);

#if defined(UTH_REUSE)
	pthread_mutex_lock(&uth->pth_mutex);
	while (1) {
		pthread_mutex_unlock(&uth->pth_mutex);
		uth->func(uth->arg);
		/* if recycling is enabled, the thread is suspended here */
	}
	pthread_mutex_unlock(&uth->pth_mutex);
#else
	uth->func(uth->arg);
#endif

	return 0;
}

void uth_switchto_ex(int vp, struct _uth_t *old, struct _uth_t *new)
{
	new->vp_id = vp;

	if (old == NULL) {
#if defined(UTH_REUSE)
		struct _uth_t *old_uth = uth_self();
		pthread_mutex_lock(&old_uth->pth_mutex);	/* here for better safety */
#endif

		pthread_mutex_lock(&new->pth_mutex);
		if (new->pth_created == 0) {
			pthread_create(&new->pth_handler, &attr, _uth_main, new);
			new->pth_created = 1;
		}
		else {
			pthread_cond_signal(&new->pth_cond);	
		}
		pthread_mutex_unlock(&new->pth_mutex);


		/* I am going to exit */
#if defined(UTH_REUSE)
		pthread_cond_wait(&old_uth->pth_cond, &old_uth->pth_mutex);
		pthread_mutex_unlock(&old_uth->pth_mutex);
#endif
	}
	else {
		pthread_mutex_lock(&old->pth_mutex);	/* here for better safety */

		pthread_mutex_lock(&new->pth_mutex);

		if (new->pth_created == 0) {
			pthread_create(&new->pth_handler, &attr, _uth_main, new);
			new->pth_created = 1;
		}
		else {
			pthread_cond_signal(&new->pth_cond);
		}
		pthread_mutex_unlock(&new->pth_mutex);

		pthread_cond_wait(&old->pth_cond, &old->pth_mutex);
		pthread_mutex_unlock(&old->pth_mutex);
	}
}


#if defined(WIN32_MSVC)
int getpagesize()
{
	return 4096;
}
#elif defined(QNX)
size_t getpagesize(void)
{
	return 4096;
}
#endif

void uth_reinit(uth_t thread, void (*start)(void *arg), void *arg)
{
	thread->func = start;
	thread->arg = arg;
}


void uth_swap_thread_context(struct _uth_t **t1, struct _uth_t **t2)
{
	struct _uth_t *temp;

	temp = *t1;
	*t1 = *t2;
	*t2 = temp;
}


void uth_set_alloc_routine(void *func)
{
	/* not used */
}


/* 
 * Return elapsed time in sec
 */

#if !defined(WIN32_MSVC) && !defined(MINGW)
#include <sys/time.h>
#include <time.h>
#endif

double uth_gettime(void)
{
#if defined(WIN32_MSVC) || defined(MINGW)
	return (double) GetTickCount()*0.001;
#else
	struct timeval t;
	gettimeofday(&t, NULL);
	return (double)t.tv_sec + (double)t.tv_usec*1.0E-6;
#endif
}

/* 
 * Sleep for ms milliseconds (kernel level)
 */
void uth_vp_sleep(int ms)
{
#if defined(WIN32_MSVC) || defined(MINGW)
	Sleep(ms);
#else
	struct timespec ts;

	ts.tv_sec = ms/1000;
	ts.tv_nsec = (ms%1000)*1E6;
	nanosleep(&ts,NULL);
#endif
}

unsigned long uth_vp_self(void)
{
	/* Not applicable here */
#if !defined(WIN32_MSVC) && !defined(MINGW)
	return (unsigned long) pthread_self();
#else
	return (unsigned long) pthread_getw32threadhandle_np(pthread_self());
#endif
}
