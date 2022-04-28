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

/*! \file uth_test.c
    \brief Base application for checking UthLib (see documentation file)
 */
 
#include <pthread.h>
#include "uth.h"
#include <stdio.h>

#define NumThreads 2

uth_t worker[NumThreads];
uth_t virtualprocessorA;
uth_t virtualprocessorB;

int gvar = 0;

void workerFunc(void* arg)
{
	long id = (long)arg;
	int lvar = 0;

	gvar++; lvar++;

	printf("[0x%lx] round one: arg (%ld / %ld)\t [local = %d global = %d]\n",
		(unsigned long) uth_self(), id, (long) uth_getarg(uth_self()), lvar, gvar); 
	if (id == NumThreads-1)
		uth_switchto_ex(0, worker[id], virtualprocessorA);
	else
		uth_switchto_ex(0, worker[id], worker[id+1]);

	gvar++; lvar++;

	printf("[0x%lx] round two: arg (%ld / %ld)\t [local = %d global = %d]\n",
		(unsigned long) uth_self(), id, (long) uth_getarg(uth_self()), lvar, gvar);

	if (id == NumThreads-1)
		uth_switchto_ex(1, NULL, virtualprocessorB);
	else
		uth_switchto_ex(1, NULL, worker[id+1]);
}

void *kernelthreadfunc(void *arg)
{
	uth_vp_init(1, NULL);
	virtualprocessorB = uth_self();

	printf("[0x%lx] master thread B starts\t [pthread_t = 0x%lx]\n",
		(unsigned long) uth_self(), uth_vp_self());

	uth_switchto_ex(1, virtualprocessorB, worker[0]);

	printf("[0x%lx] master thread B exits\t [global = %d]\n",
		(unsigned long) uth_self(), gvar);


	return 0;
}

int main(void)
{
	long i;
	pthread_t pth;
	pthread_attr_t attr;
	long status;

	pthread_attr_init(&attr);
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	uth_info();

	uth_init(0);
	uth_vp_init(0, NULL);

	for(i=0; i<NumThreads; ++i) worker[i] = uth_create(workerFunc,(void *)i);

	printf("[0x%lx] master thread A starts\t [pthread_t = 0x%lx]\n",
		(unsigned long) uth_self(), uth_vp_self());
	virtualprocessorA = uth_self();
	uth_switchto_ex(0, virtualprocessorA, worker[0]);
	printf("[0x%lx] master thread A continues\t [global = %d]\n",
		(unsigned long) uth_self(), gvar);

	pthread_create(&pth, &attr, kernelthreadfunc, NULL);
	pthread_join(pth, (void *)&status);

	for(i=0; i<NumThreads; ++i)		uth_delete(worker[i]);

	printf("[0x%lx] master thread A exits\n", (unsigned long) uth_self());
	return (gvar == (2*NumThreads));
}
