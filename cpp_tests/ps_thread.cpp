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

extern "C"
{
#include <psthread.h>
void uth_info(void);
}
#include <stdio.h>
#include <stdlib.h>

static psthread_t maintid;


struct ThreadArgs {
  int L;
  int G;
  bool M;
  psthread_t SysThread;
  ThreadArgs(int localId, int globalId, bool Master, psthread_t S)
      : L(localId), G(globalId), M(Master), SysThread(S){};
  ThreadArgs() {}
};


class Thread
{
	Thread * MyThread; /* How we get here... */
	int LocalId;
	int GlobalId;
	psthread_t SystemThread;
	Thread(ThreadArgs const *);

public:
	static void createThread(ThreadArgs *);
	auto getCurrentThread() {
		return MyThread;
	}
	auto getLocalId() const {
		return LocalId;
	}
	auto getGlobalId() const {
		return GlobalId;
	}

	static void mythread(void *arg)
	{
		ThreadArgs *a = (ThreadArgs *)arg;
		int i;
		psthread_t t = psthread_self();
		fprintf(stderr, "thread(%ld) started on vp %d. [stack = %p] \n", t, psthread_current_vp(), (char *)&i);
		return;
	}

};

//Thread * Thread::MyThread;

void Thread::createThread(ThreadArgs * Args) {
  if (Args->M) {
    new Thread(Args);
  }
  else {
    // Child thread needs to be created
    printf("Creating std::thread for %d\n", Args->L);
    //Args->SysThread = new std::thread(outerLoop, Args);
    psthread_create(&Args->SysThread, NULL, mythread, (void *)Args);

    // The Thread object is created by the the new thread itself.
    // And allowed to run asynchronously.
    // Until we set this the new thread cannot execute, so the assignment
    // above should not be a race. The code in outerLoop which looks
    // at Args.SysThread cannot execute before we have assigned the value.
    //Args->SysThread->detach();
    // Thread data structure initialization happens at the beginning of outerLoop, since
    // it needs to set the appropriate thread static data, so has to be executed
    // by the thread itself, and allocating the thread data structures inside the thread may
    // improve performance in a NUMA system if they are allocated first touch.
  }
}


Thread::Thread(ThreadArgs const * Args)
    : LocalId(Args->L), GlobalId(Args->G),
      SystemThread(Args->SysThread) {
  // Initialize the thread static variable so we can find ourselves again.
  MyThread = this;
  // And set our place in the team.

  printf("Created thread %d with system thread %p", LocalId,
        SystemThread);

}


void mythread(void *arg)
{
	long t= (long)arg;
	int i;

	fprintf(stderr, "thread(%ld) started on vp %d. [stack = %p] \n", t, psthread_current_vp(), (char *)&i);
//	psthread_vp_sleep(1000*(t+1));
	fprintf(stderr, "thread(%ld) finished on vp %d. [stack = %p] \n", t, psthread_current_vp(), (char *)&i);
}

#define USER_LEVEL_THREADS	2
#define VIRTUAL_PROCESSORS	2

int main(int argc, char *argv[])
{
	double t0, t1;
	psthread_t tid[1] = {0};
	long i;
	psthread_version();
	printf("main stack = %p\n", (char *)&i);
	uth_info();

	psthread_init(VIRTUAL_PROCESSORS, 0);
	maintid=psthread_self();

	printf("main stack = %p\n", (char *)&i);
	fprintf(stderr, "[0x%lx]: main thread=%p\n", psthread_vp_self(), maintid);

	//psthread_create(&tid[0],NULL,mythread,(void *)0);

	ThreadArgs *Args = new ThreadArgs(1, 1, false, 0);
        Thread::createThread(Args);

  // Then spawn the others. This could be done recursively,
  // but there are kernel locks which mean that's probably not worthwhile,
  // and this only happens once at the moment.

	t0 = psthread_gettime();

	psthread_enqueue(Args->SysThread, -1);
	psthread_waitall();

#if 0

	for (i = 0; i < USER_LEVEL_THREADS; i++) {
		psthread_create(&tid[i],NULL,mythread,(void *)i);
		fprintf(stderr, "[0x%lx]: tid %p created\n", psthread_vp_self(), tid[i]);
		psthread_enqueue(tid[i], -1);
//		psthread_enqueue(tid[i], i % VIRTUAL_PROCESSORS);
	}

	printf("Before block...\n");
	psthread_waitall();
#endif
	t1 = psthread_gettime();
	fprintf(stderr, "[0x%lx]: Elapsed Time =%f seconds\n", psthread_vp_self(), t1 - t0);
	return(0);
}

