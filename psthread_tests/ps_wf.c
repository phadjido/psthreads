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

/*! \file ps_block.c
    \brief Checks user-level thread migration in PSthreads
 */

#include <psthread.h>
#include <stdio.h>

extern void kthread_sleep(int);

static psthread_t maintid;
static int count=0;
psthread_lock_t lock;

void mythread(void *arg)
{
	long t= (long)arg;
	int i;

	fprintf(stderr, "[0x%lx]: thread %p (arg=%ld) started on vp %d. \n", psthread_vp_self(), psthread_self(),t, psthread_current_vp());

	sleep(1);
	psthread_lock_acquire(&lock);
	count++;
	psthread_lock_release(&lock);

	fprintf(stderr, "[0x%lx]: thread %p (arg=%ld) finished on vp %d\n", psthread_vp_self(), psthread_self(), t, psthread_current_vp());
}

#define VIRTUAL_PROCESSORS	2

int main(int argc, char *argv[])
{
	double t0, t1;
	psthread_t tid0, tid1, tid2, tid3;
	long i;
	
	psthread_init(VIRTUAL_PROCESSORS, 0);
	maintid= psthread_self();
	fprintf(stderr, "[0x%lx]: main thread=%p\n", psthread_vp_self(), maintid);

	psthread_lock_init(&lock);

	t0 = psthread_gettime();
	
	psthread_create(&tid0, NULL, mythread,(void *)0);
	fprintf(stderr, "[0x%lx]: tid %p created\n", psthread_vp_self(), tid0);
//	psthread_enqueue(tid0, 0);
	psthread_switch_and_yield(maintid, tid0);

	psthread_create(&tid1, NULL, mythread,(void *)1);
	fprintf(stderr, "[0x%lx]: tid %p created\n", psthread_vp_self(), tid1);
//	psthread_enqueue(tid1, 0);
	psthread_switch_and_yield(maintid, tid1);

	psthread_create(&tid2, NULL, mythread,(void *)2);
	fprintf(stderr, "[0x%lx]: tid %p created\n", psthread_vp_self(), tid2);
//	psthread_enqueue(tid2, 0);
	psthread_switch_and_yield(maintid, tid2);

	psthread_create(&tid3, NULL, mythread,(void *)3);
	fprintf(stderr, "[0x%lx]: tid %p created\n", psthread_vp_self(), tid3);
//	psthread_enqueue(tid3, 0);
	psthread_switch_and_yield(maintid, tid3);

	printf("Before block...\n");

	psthread_waitall();

	t1 = psthread_gettime();
	fprintf(stderr, "[0x%lx]: Elapsed Time =%f seconds\n", psthread_vp_self(), t1 - t0);
	fprintf(stderr, "[0x%lx]: main finished. count=%d\n", psthread_vp_self(), count);
	
	psthread_lock_destroy(&lock);

	return(0);
}

