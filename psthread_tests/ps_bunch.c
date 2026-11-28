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

static unsigned long ctr;

void mythread(void *arg)
{
	long t= (long)arg;
	int i;

//	printf("%d\n", t);
//	sleep(1);
//	ctr++;
}

#define USER_LEVEL_THREADS	16
//00000
#define VIRTUAL_PROCESSORS	8

#define INITER		10000
int main(int argc, char *argv[])
{
	double t0, t1;
	long i;
	psthread_t t;
	psthread_t tt[USER_LEVEL_THREADS]={0};
	int j, k;

	psthread_init(VIRTUAL_PROCESSORS, 0);

	fprintf(stderr, "[0x%lx]: main thread=%p\n", psthread_vp_self(), psthread_self());

	for (k = 0; k < 10; k++)
	{
	t0 = psthread_gettime();
	for (j = 0; j < INITER; j++)
	{
		ctr = 0;
		for (i = 0; i < USER_LEVEL_THREADS; i++) {
			psthread_create_ex(&tt[i],NULL,mythread,(void *)i);
//			psthread_enqueue(tt[i], 0);
		}
#if 0
		for (i = 0; i < USER_LEVEL_THREADS; i++) {
			psthread_enqueue(tt[i], -1);
		}
#else
		psthread_enqueue_bunch_tail(tt, USER_LEVEL_THREADS, -1);
#endif
		psthread_waitall();
	}
	t1 = psthread_gettime();
	fprintf(stderr, "B [0x%lx]: Elapsed Time for %d threads =%f seconds [counter=%ld]\n", psthread_vp_self(), USER_LEVEL_THREADS*INITER, t1-t0, ctr);
//	printf("\n\n");
	
	}
	return(0);
}

