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
    \brief Measures context-switch overhead
 */

#include <uth.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#define THREADS 3
#define TIMES	10


#define VALIDATE

unsigned int flags[THREADS*TIMES];
int counter = 0;

uth_t uth[THREADS];

void func (void *arg)
{
	register unsigned long i = 0, self = (unsigned long) arg;
	register unsigned long next = (self+1)%THREADS;
	uth_t me = uth_self();
/*	printf("self = %d\n", self);*/

	for (i = 0; i < TIMES; i++)
	{
#ifdef VALIDATE
		flags[counter] = self;
/*		printf("[0x%lx] (%ld)\n", me, self);*/
		counter++;
#endif
		uth_switchto_ex(0, uth[self], uth[next]);
/*		uth_switchto(uth[self], uth[next]);*/
	}
}

int main (int argc, char * argv [])
{
	unsigned long i;
	double time1, time2;
	double elapsed;
	char *sp;
	int size;

	printf ("Executing in nth_main \n");

	uth_init(0);
	uth_vp_init(0, NULL);
	uth[0] = uth_self_ex(0);

	size = uth_get_stacksize();
	printf("size = %d\n", size);
	for (i = 1; i < THREADS; i++)
	{
		posix_memalign(&sp, size, size);
		printf("stack: %p\n", sp);
		uth[i] = uth_create_sp(func, (void *)i, size, sp);
	}

#if 1
	time1 = uth_gettime();
	func(0);
	time2 = uth_gettime();

	elapsed = (time2 - time1)*1E6;
	printf("%.6f usecs  [%.6f usecs per context-switch]\n", elapsed, (elapsed)/(THREADS*TIMES));

	for (i = 1; i < THREADS; i++)
	{
		uth_delete(uth[i]);
	}

	for (i = 0; i < THREADS*TIMES; i++)
	{
/*		printf("%d", flags[i]);*/
		if (flags[i] != (i % THREADS))
		{
			printf("error at %ld: found %d instead of %ld\n", i, flags[i], i % THREADS);
			exit(1);
		}
	}
#endif
	printf("uth_create_sp worked successfully\n");

	return 0;
}
