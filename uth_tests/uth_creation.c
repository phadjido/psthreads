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

/*! \file uth_ctxsw.c
    \brief Measures thread creation overhead
 */
#include <uth.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef WIN32
#include <sys/types.h>
#include <sys/time.h>
#else
#include <windows.h>
#endif

#define THREADS 100
#define WARM_UP_ROUND

uth_t uth[THREADS];

void func (void *arg)
{
}

int main (int argc, char * argv [])
{
	unsigned long i;
	double time1, time2;
	double elapsed;
	unsigned long stacksize = 0;

	printf ("Executing in nth_main \n");

	if (argc > 1) stacksize = atol(argv[1]);

	printf("stacksize = %ld\n", stacksize);
	uth_init(stacksize);
	uth_vp_init(0, NULL);

/*#ifdef WARM_UP_ROUND*/
	time1 = uth_gettime();
	for (i = 0; i < THREADS; i++) {
		uth[i] = uth_create(func, (void *)i);
	}
	time2 = uth_gettime();

	elapsed = (time2 - time1)*1E6;
	printf("%.6f usecs  [%.6f usecs per thread-creation]\n", elapsed, (elapsed)/(THREADS));
	printf("\n");

	for (i = 0; i < THREADS; i++) {
		uth_delete(uth[i]);
	}
/*#endif*/

	time1 = uth_gettime();
	for (i = 0; i < THREADS; i++) {
		uth[i] = uth_create(func, 0);
	}
	time2 = uth_gettime();

	elapsed = (time2 - time1)*1E6;
	printf("%.6f usecs  [%.6f usecs per thread-reuse]\n", elapsed, (elapsed)/(THREADS));
	printf("\n");

	time1 = uth_gettime();
	for (i = 0; i < THREADS; i++) {
		uth_reinit(uth[i], func, 0);
	}
	time2 = uth_gettime();

	elapsed = (time2 - time1)*1E6;
	printf("%.6f usecs  [%.6f usecs per thread-reinitialization]\n", elapsed, (elapsed)/(THREADS));
	printf("\n");

	return 0;
}
