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

	ctr++;
}

#define USER_LEVEL_THREADS	10
//0000
#define VIRTUAL_PROCESSORS	4


int main(int argc, char *argv[])
{
	double t0, t1;
	long i;
	psthread_t t;
	psthread_t tt[USER_LEVEL_THREADS]={0};
	int j, k;

	psthread_init(VIRTUAL_PROCESSORS, 0);

	fprintf(stderr, "[0x%lx]: main thread=%p\n", psthread_vp_self(), psthread_self());

	for (k = 0; k < 2; k++)
	{
//#if 0
	for (j = 0; j < 10; j++)
	{
		ctr = 0;
		t0 = psthread_gettime();
		for (i = 0; i < USER_LEVEL_THREADS; i++) {
			psthread_create(&t,NULL,mythread,(void *)i);
			psthread_enqueue(t, -1);
		}
		psthread_waitall();
		t1 = psthread_gettime();
		fprintf(stderr, "A [0x%lx]: Elapsed Time for %d threads =%f seconds [counter=%ld]\n", psthread_vp_self(), USER_LEVEL_THREADS, t1-t0, ctr);
	}

	printf("\n");
//#else
	for (j = 0; j < 10; j++)
	{
		ctr = 0;
		t0 = psthread_gettime();
		for (i = 0; i < USER_LEVEL_THREADS; i++) {
			psthread_create_ex(&tt[i],NULL,mythread,(void *)i);
			psthread_enqueue(tt[i], -1);
		}
		psthread_waitall();
		t1 = psthread_gettime();
		fprintf(stderr, "B [0x%lx]: Elapsed Time for %d threads =%f seconds [counter=%ld]\n", psthread_vp_self(), USER_LEVEL_THREADS, t1-t0, ctr);
	}
//#endif
	printf("\n\n");
	
	}
	return(0);
}

