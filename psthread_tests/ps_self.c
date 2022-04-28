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

#define USER_LEVEL_THREADS	10000000
#define VIRTUAL_PROCESSORS	8

psthread_t me;

#define psthread_self_stk	psthread_self

void routine(void *arg)
{
	int i, k, j;
	double t0, t1;

	fprintf(stderr, "[0x%lx]: bench thread=%p %p on %d\n", psthread_vp_self(), psthread_self(), psthread_self_stk(), psthread_current_vp());

	for (k = 0; k < 2; k++)
	{
	for (j = 0; j < 10; j++)
	{
		t0 = psthread_gettime();
		for (i = 0; i < USER_LEVEL_THREADS; i++) {
			me = psthread_self();
			//me = psthread_self_stk();
		}
		psthread_waitall();
		t1 = psthread_gettime();
		fprintf(stderr, "A [0x%lx]: Elapsed Time for %d calls =%f seconds [mean=%.1lf nsecs]\n", psthread_vp_self(), USER_LEVEL_THREADS, t1-t0, (t1-t0)*1.0E9/USER_LEVEL_THREADS);
	}
	printf("\n\n");
	
	}
}

int main(int argc, char *argv[])
{
	long i;
	psthread_t t;
	int vp = 0;

	if (argc == 2) vp = atoi(argv[1]);

	psthread_init(VIRTUAL_PROCESSORS, 0);

	fprintf(stderr, "[0x%lx]: main thread=%p on %d\n", psthread_vp_self(), psthread_self(), psthread_current_vp());

	psthread_create(&t, NULL, routine, NULL);
	psthread_enqueue(t, vp);
	psthread_waitall();

	psthread_stats();

	return(0);
}

