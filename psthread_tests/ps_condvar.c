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

/*! \file ps_condvar.c
    \brief Checks psthreads condition variables
 */

#include "psthread.h"
#include <stdio.h>

extern void kthread_sleep(int);

static psthread_t maintid;
static int started = 0, count=0;
psthread_lock_t lock;

psthread_cond_t co;
psthread_lock_t lo;

void mythread(void *arg)
{
	long t= (long)arg;

#if 1
	psthread_vp_sleep(10);
#else
	psthread_yield();
#endif

	fprintf(stderr, "[%lx]: thread %p (arg=%ld) started on vp %d. \n", psthread_vp_self(), psthread_self(),t, psthread_current_vp());


	psthread_lock_acquire(&lo);
	started++;
	psthread_cond_wait(&co, &lo);
	psthread_lock_release(&lo);

#if 0
	psthread_vp_sleep(100);
#else
	psthread_yield();
#endif

	psthread_lock_acquire(&lock);
	count++;
	psthread_lock_release(&lock);

	fprintf(stderr, "[%lx]: thread %p (arg=%ld) finished on vp %d\n", psthread_vp_self(), psthread_self(), t, psthread_current_vp());
	
}

#define USER_LEVEL_THREADS	16
#define VIRTUAL_PROCESSORS	4

int main(int argc, char *argv[])
{
	double t0, t1;
	psthread_t tid[USER_LEVEL_THREADS];
	long i;
	
	psthread_init(VIRTUAL_PROCESSORS, 0);
	maintid= psthread_self();
	fprintf(stderr, "[%lx]: main thread=%p\n", psthread_vp_self(), maintid);

	psthread_lock_init(&lock);

	psthread_cond_init(&co);
	psthread_lock_init(&lo);

	t0 = psthread_gettime();
	for (i = 0; i < USER_LEVEL_THREADS; i++) {
		psthread_create(&tid[i], NULL, mythread,(void *)i);
		fprintf(stderr, "[%lx]: tid %p created\n", psthread_vp_self(), tid[i]);
		psthread_enqueue(tid[i], -1);
		fprintf(stderr, "[%lx]: tid %p enqueued\n", psthread_vp_self(), tid[i]);
	}

	while (started < USER_LEVEL_THREADS)  {
		psthread_vp_sleep(10);
/*		psthread_yield();*/
	}
		
	printf("started = %d\n", started);

	psthread_vp_sleep(1000);

/*	for (i = 0; i < USER_LEVEL_THREADS; i++) {
 *		printf("Signaling one!\n");
 *		psthread_cond_signal(&co);
 *		kthread_sleep(100);
 *	}
 */

	psthread_lock_acquire(&lo);
	psthread_cond_broadcast(&co);
	psthread_lock_release(&lo);

	psthread_waitall();
	
	t1 = psthread_gettime();
	
	fprintf(stderr, "[%lx]: main finished. count=%d\n", psthread_vp_self(), count);
	fprintf(stderr, "[%lx]: Elapsed Time =%f seconds\n", psthread_vp_self(), t1 - t0);
	
	psthread_lock_destroy(&lock);

	psthread_stats();

	return(0);
}

