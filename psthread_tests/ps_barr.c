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

/*! \file ps_test.c
    \brief Simple application case
 */

#include <psthread.h>
#include <stdio.h>
#include <stdlib.h>

static psthread_t maintid;

static psthread_barrier_t bar;

void mythread(void *arg)
{
	long t= (long)arg;
	int i;

	fprintf(stderr, "thread(%ld) started on vp %d. [stack = %p] \n", t, psthread_current_vp(), (char *)&i);

	for (i = 0; i < 10; i++) {
		psthread_vp_sleep(100);
		psthread_barrier_wait(&bar);
//		psthread_barrier_wait_ex(&bar, t);
		fprintf(stderr, "[%d]: thread(%ld) running on vp %d. \n", i, t, psthread_current_vp());
	}
	psthread_barrier_wait(&bar);
//	psthread_barrier_wait_ex(&bar, t);

	fprintf(stderr, "thread(%ld) finished on vp %d\n", t, psthread_current_vp());
}

#define USER_LEVEL_THREADS	4
#define VIRTUAL_PROCESSORS	1

int main(int argc, char *argv[])
{
	double t0, t1;
	psthread_t tid[USER_LEVEL_THREADS];
	long i;

	printf("main stack = %p\n", (char *)&i);
	
	psthread_init(VIRTUAL_PROCESSORS, 0);

	maintid=psthread_self();

	printf("main stack = %p\n", (char *)&i);

	fprintf(stderr, "[0x%lx]: main thread=%p\n", psthread_vp_self(), maintid);

	psthread_barrier_init(&bar, USER_LEVEL_THREADS);
//	psthread_barrier_init_ex(&bar, USER_LEVEL_THREADS);

	fprintf(stderr, "After barrier_init\n");

	t0 = psthread_gettime();

	for (i = 0; i < USER_LEVEL_THREADS; i++) {
		psthread_create(&tid[i],NULL,mythread,(void *)i);
		fprintf(stderr, "[0x%lx]: tid %p created\n", psthread_vp_self(), tid[i]);
		psthread_enqueue(tid[i], -1);
	}

	printf("Before block...\n");
	psthread_waitall();
	
	t1 = psthread_gettime();
	
	fprintf(stderr, "[0x%lx]: Elapsed Time =%f seconds\n", psthread_vp_self(), t1 - t0);
	
	return(0);
}

