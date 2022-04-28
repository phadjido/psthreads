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

/*! \file psthread.c
    \brief PSthreads runtime library
 */

#include "common.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <uth.h>

typedef union _vp_psthread {
	struct {
		psthread_t *__ps_unlock_thread;		/* Thread to have its struct unlocked */
		psthread_t *__ps_yield_thread;		/* Thread to have its struct unlocked */
		psthread_t *__ps_enqueue_thread;	/* Thread to be enqueued */
		psthread_t *__ps_scheduler_thread;	/* Scheduler thread for this vp */
	} thr;
	char pad[CACHE_LINE_SIZE];
} vp_psthread_t;

static struct _ps_global_data {
	vp_psthread_t		__ps_vp_psthr[MAX_VPS];

	queue_t			__ps_prive_ready_queue[MAX_VPS];
	queue_t			__ps_local_ready_queue[MAX_VPS];
	queue_t			__ps_lreuse_queue[MAX_VPS];
	queue_t			__ps_ready_queue;
	queue_t 		__ps_reuse_queue;

	pthread_t		__ps_vpid[MAX_VPS];

	unsigned long		__thread_created[MAX_VPS][CACHE_LINE_SIZE/SIZEOF_LONG];
	unsigned long		__thread_reused[MAX_VPS][CACHE_LINE_SIZE/SIZEOF_LONG];

	int			__ps_nvps[CACHE_LINE_SIZE/SIZEOF_INT];
	int			__ps_vp_created[CACHE_LINE_SIZE/SIZEOF_INT];
	int			__ps_stealing_env[CACHE_LINE_SIZE/SIZEOF_INT];
	int			__ps_stealing[CACHE_LINE_SIZE/SIZEOF_INT];
} *_ps_gdata;

#define _ps_unlock_thread(x)	_ps_gdata->__ps_vp_psthr[x].thr.__ps_unlock_thread
#define _ps_yield_thread(x)	_ps_gdata->__ps_vp_psthr[x].thr.__ps_yield_thread
#define _ps_enqueue_thread(x)	_ps_gdata->__ps_vp_psthr[x].thr.__ps_enqueue_thread
#define _ps_scheduler_thread(x)	_ps_gdata->__ps_vp_psthr[x].thr.__ps_scheduler_thread

#define _ps_prive_ready_queue	_ps_gdata->__ps_prive_ready_queue
#define _ps_local_ready_queue	_ps_gdata->__ps_local_ready_queue
#define _ps_ready_queue		_ps_gdata->__ps_ready_queue
#define _ps_lreuse_queue	_ps_gdata->__ps_lreuse_queue
#define _ps_reuse_queue		_ps_gdata->__ps_reuse_queue

#define _ps_thread_created(x)	_ps_gdata->__thread_created[x][0]
#define _ps_thread_reused(x)	_ps_gdata->__thread_reused[x][0]

#define _ps_vpid		_ps_gdata->__ps_vpid

#define _ps_nvps		_ps_gdata->__ps_nvps[0]
#define _ps_vp_created		_ps_gdata->__ps_vp_created[0]
#define _ps_stealing_env	_ps_gdata->__ps_stealing_env[0]
#define _ps_stealing		_ps_gdata->__ps_stealing[0]

int _ps_stacksize; 
int _ps_bindcpu;
int _ps_ldq = 1;	/*  local dequeue: 1-> Head, 0-> End */
int _ps_rdq = 0;	/* remote dequeue: 1-> Head, 0-> End */
int _ps_gdq = 1;	/* global dequeue: 1-> Head, 0-> End */
pthread_mutex_t gm = PTHREAD_MUTEX_INITIALIZER;

#if defined(PS_STATS)
static unsigned long 		_stl_att[MAX_VPS][CACHE_LINE_SIZE/SIZEOF_LONG];
static unsigned long		_stl_hit[MAX_VPS][CACHE_LINE_SIZE/SIZEOF_LONG];
static unsigned long 		_stl_hit2[MAX_VPS][MAX_VPS+1][CACHE_LINE_SIZE/SIZEOF_LONG];
#endif

#define _PS_WORK_STEALING_DISABLED	0
#define _PS_WORK_STEALING_ENABLED	1
#define _PS_WORK_STEALING_DYNAMIC	2

int psthread_stealing(int status)
{
	int res = _ps_stealing;

	_ps_stealing = status;
	return res;
}

//static psthread_attr_t default_attr = PSTHREAD_ATTR_INITIALIZER;

/* forward declarations */
int psthread_create (psthread_t **t, psthread_attr_t *attr, void (*start)(void *arg), void *arg);
/*inline*/ psthread_t *psthread_self();
/*inline*/ psthread_t *psthread_self_ex(int vp);
/*inline*/ psthread_t *psthread_self_stk();
static void _ps_thread_main(void *arg);


/*
 * High-level lock primitives 
 */

void psthread_lock_init(psthread_lock_t *spin)
{
	_lock_init(spin);
}

int psthread_lock_acquire(psthread_lock_t *spin)
{
	_lock_acquire(spin);
	return 0;
}

int psthread_lock_try_acquire(psthread_lock_t *spin)
{
	return _lock_try_acquire(spin);
}

int psthread_lock_release(psthread_lock_t *spin)
{
	_lock_release(spin);
	return 0;
}

void psthread_lock_destroy(psthread_lock_t *spin)
{
	_lock_destroy(spin);
}

/*
 * Implementation of the body of the scheduling loop. It is continuously
 * executed by each virtual processor. It does some cleanup work, selects
 * the next user-level thread to be executed and context-switches to it.
 */ 

static inline psthread_t *stealing(int vp)
{  
	int i, j;
	psthread_t *thread = NULL;

#if 0
	return NULL;
#endif

#if 0
	i = rand()% _ps_nvps;
	if (i != vp) {
		_dequeue_end(&_ps_local_ready_queue[i], &thread);
#if defined(PS_STATS)
		if (thread) _stl_hit2[vp][i][0]++;
#endif
	}
	return thread;
#endif

#if 0
	i = (vp + 1) % _ps_nvps;
	while ((thread == NULL) && (i != vp)) {
		if (!_ps_rdq)	{ _dequeue_end(&_ps_local_ready_queue[i], &thread);}
		else		{ _dequeue(&_ps_local_ready_queue[i], &thread);}
#if defined(PS_STATS)
		if (thread) _stl_hit2[vp][i][0]++;
#endif
		i = (i + 1) % _ps_nvps;
	}                  
#endif

#if 1
	int GroupSize = 2;
	int GroupId, GroupBaseVP;
	char visited[16];
	memset(visited, 0, 16);
	while (GroupSize <= _ps_nvps) {
		GroupId = vp / GroupSize;
		GroupBaseVP = GroupId*GroupSize;
		i = (vp + 1) % GroupSize + GroupBaseVP;
		while ((thread == NULL) && (i != vp)) {
			if (!visited[i]) {
				visited[i] = 1;
				if (!_ps_rdq)	{ _dequeue_end(&_ps_local_ready_queue[i], &thread);}
				else		{ _dequeue(&_ps_local_ready_queue[i], &thread);}
#if defined(PS_STATS)
				if (thread) _stl_hit2[vp][i][0]++;
#endif
			}
			i = (i + 1) % GroupSize + GroupBaseVP;
		}
		GroupSize *= 2;
	}
#endif

	return thread;
}

static inline void _ps_recycle(psthread_t *thread, int context)
{
	int vp = thread->vpid;
	if (context) uth_delete(thread->context);
	thread->context = NULL;
	thread->state = _PS_FREED;
	_lock_release(&thread->lock);   /* is it required ? */

	if (thread->staticmem == 0xFABE) return;
#if defined(UTH_REUSE)
#if defined(UTH_LOCAL_REUSE)
	_enqueue_head_bare(&_ps_lreuse_queue[vp], thread);
#else
	_enqueue_head(&_ps_reuse_queue, thread);
#endif
#else
	//free(thread->realmem);        /* is missing */
#endif
}

#define _ps_recycle_cleanup(me, uth)		\
	do {					\
	if (me->cleanup != NULL) {		\
		_ps_recycle(me->cleanup, uth);	\
		me->cleanup = NULL;		\
	}					\
	} while(0);

static inline void _ps_vp_schedule_stage1(long vp)
{
	psthread_t *unlock_thr = NULL;

	unlock_thr = _ps_unlock_thread(vp);
	if (unlock_thr != NULL) {
		_ps_unlock_thread(vp) = NULL;

		//_lock_release(&unlock_thr->lock);	
		if (unlock_thr->state == _PS_ZOMBIE) {
			_ps_recycle(unlock_thr, 1);	/* 0??? */
		}
		else {
			_lock_release(&unlock_thr->lock);	
		}
	}

	if (_ps_yield_thread(vp) != NULL) {
#if defined(PS_GLOBAL_QUEUE_ONLY)
		_enqueue_tail(&_ps_ready_queue, _ps_yield_thread(vp));
#else
		_enqueue_tail(&_ps_local_ready_queue[vp], _ps_yield_thread(vp));
#endif
		_ps_yield_thread(vp) = NULL;
	}
}

static inline psthread_t *_ps_vp_schedule_stage2(long vp)
{
	psthread_t *thread = NULL;

	if (_ps_enqueue_thread(vp) != NULL) {
		thread = _ps_enqueue_thread(vp);	/* nice option but not appropriate for yield */
		_ps_enqueue_thread(vp) = NULL;
	}

	if (thread == NULL) _dequeue(&_ps_prive_ready_queue[vp], &thread);	/* check private queue */

#if !defined(PS_GLOBAL_QUEUE_ONLY)
	if (thread == NULL){	/* check local queue */
		if (_ps_ldq)	{ _dequeue(&_ps_local_ready_queue[vp], &thread);}
		else		{ _dequeue_end(&_ps_local_ready_queue[vp], &thread);}
	}
	//if (thread) printf("[%d] %s: %p %d\n", vp, __FUNCTION__, thread, vp);
#if defined(PS_STATS)
	if (thread) _stl_hit2[vp][vp][0]++;
#endif

#if 1
	if (thread == NULL) {	/* coarse grain */
		if (_ps_gdq)	{ _dequeue(&_ps_ready_queue, &thread);}
		else		{ _dequeue_end(&_ps_ready_queue, &thread);}
		//if (thread) printf("[%d] %s: %p %d\n", vp, __FUNCTION__, thread, -1);
#if defined(PS_STATS)
		if (thread) _stl_hit2[vp][_ps_nvps][0]++;
#endif
	}
#endif

/*#if defined(PS_STEALING)*/
	if ((thread == NULL)&&(_ps_stealing)) {
		thread = stealing(vp);
#if defined(PS_STATS)
		_stl_att[vp][0]++;
		if (thread != NULL) _stl_hit[vp][0]++;
#endif
	}
/*#endif*/

#else	/* Global queue only */
	if (thread == NULL) _dequeue(&_ps_ready_queue, &thread);	/* coarse grain */
#endif

	return thread;
}

#if 1
#include "sysdeps.h"
#define YIELD_IMMEDIATELY 0
#define YIELD_FREQUENTLY  50
#define YIELD_OCCASIONALY 150                 
#define YIELD_RARELY      500

//#define PS_WAIT_WHILE(trials_before_yielding) { \
//          volatile int time = 0; \
//          for ( ; ; time++) \
//            if (time == (trials_before_yielding)) { \
//              time = -1; \
//            }; \
//        }

//#ifdef __FENCE_MUTEX__
  #include <pthread.h>
  static pthread_mutex_t _ps_flusher = PTHREAD_MUTEX_INITIALIZER;
  void _ps_bad_fence(void)
  {
    pthread_mutex_lock(&_ps_flusher);
    pthread_mutex_unlock(&_ps_flusher);
  }
//#endif

#undef FENCE
//#define FENCE	_ps_bad_fence()
//#define FENCE asm volatile("cpuid" : : : "%eax", "%ebx", "%ecx", "%edx")
//#define FENCE asm volatile("mfence" : : : "memory");
#define FENCE


#define PS_WAIT_WHILE(f, trials_before_yielding) { \
          volatile int time = 0; FENCE; \
          for ( ; (f); time++) \
            if (time == (trials_before_yielding)) { \
              time = -1; \
              psthread_yield(); \
            }; \
        }
#endif

//#undef PS_YIELDVP
#define PS_YIELDVP_EXP	

static inline void _ps_vp_schedule(long vp, int *idlecnt)
{
	psthread_t *thread = NULL;
	int idlecnt_1 = *idlecnt-1;

	_ps_vp_schedule_stage1(vp);

//	while (_ps_stealing_env == -1) PS_WAIT_WHILE(10);
//label100:
	thread = _ps_vp_schedule_stage2(vp);

	if (thread == NULL) {
		thread = _ps_scheduler_thread(vp);
		thread->state = _PS_RUNNING;
#if defined(PS_YIELDVP)
		{
			int j; // ,brk; 
			for (j = 0/*, brk = 0*/; j < idlecnt_1; j++) {
//* abc */				if (_ps_local_ready_queue[vp].q.head != NULL) { /*brk = 1;*/ break;}
				sched_yield();
			}
//			if (brk) {
//				goto label100;
//			}
		}
#if defined(PS_YIELDVP_EXP)
		if (*idlecnt <= 32) *idlecnt *=2;
#else	/* PS_YIELDVP_LIN */
		if (*idlecnt <= 2) *idlecnt +=1;
#endif
#else
		{
		struct timespec req, rem;
		req.tv_sec = 0;
		req.tv_nsec = 10;
		nanosleep(&req, &rem); 
		}
#endif
		//printf("[%d] Idle...\n", vp); sleep(1);
		return;
	}
	*idlecnt = 1;

	/* Resume the thread */
	thread->state = _PS_RUNNING;
	thread->vpid = vp;
	if (thread->context == NULL) {
		thread->context = uth_create(_ps_thread_main, thread);
	}

	uth_switchto_ex(vp, _ps_scheduler_thread(vp)->context, thread->context);
}

/* 
 * Voluntarily release your virtual processor
 */
void psthread_yield()
{
	psthread_t *me = psthread_self();
	int vp = me->vpid;

	me->state = _PS_RUNNABLE;
	_ps_yield_thread(vp) = me;
	uth_switchto_ex(vp, me->context, _ps_scheduler_thread(vp)->context);
#if defined(PS_P2P_HANDOFF)
	_ps_recycle_cleanup(me, 1);
#endif
}

#if 1
/*static*/ void psthread_switch_to_scheduler()
{
        int vp = uth_get_vpid();       
        psthread_t *me = psthread_self_ex(vp);

        me->state = _PS_RUNNABLE;
        uth_switchto_ex(vp, me->context, _ps_scheduler_thread(vp)->context);       
}

static void psthread_switch(psthread_t *from, psthread_t *to)
{
        int vp = uth_get_vpid();
        psthread_t *me = from;

        me->state = _PS_RUNNABLE;
        to->state = _PS_RUNNING;              
        to->vpid = vp;
        if (to->context == NULL) {     
                to->context = uth_create(_ps_thread_main, to);
        }
        uth_switchto_ex(vp, me->context, to->context);
}

/*static*/ void psthread_switch_and_yield(psthread_t *from, psthread_t *to)
{
        int vp = uth_get_vpid();
        psthread_t *me = from;

	to->toq = me;
        me->state = _PS_RUNNABLE;
        to->state = _PS_RUNNING;
        to->vpid = vp;
        if (to->context == NULL) {     
                to->context = uth_create(_ps_thread_main, to);
        }
        uth_switchto_ex(vp, me->context, to->context);
}

pthread_t psthread_vpid(int vp)
{                  
        return _ps_vpid[vp];
}  

#endif

/*
 * Scheduling loop
 */

void _ps_scheduler_thread_start(void *arg)
{
	long vp = (long)arg;
	int idlecnt = 1;

	while (1) {
		_ps_vp_schedule(vp, &idlecnt);
	}
}


int phys_cpu[8] = {0, 6, 1, 4, 2, 5, 3, 7};	/* paraguay */

void _bind_cpu(int vp)                
{
	int cpu;
	unsigned long mask;
	if (!_ps_bindcpu) return;
#ifdef PTHREAD_AFFINITY
#if 1
	cpu = vp;
#else
	cpu = phys_cpu[vp];
#endif
	printf("binding vp %d on cpu %d\n", vp, cpu);
	mask = (1 << cpu);

	/* bind process to processor 0 */
	if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) <0) {
		perror("pthread_setaffinity_np");
	}
#else
#endif
}

/*
 * Initialization of virtual processor
 */
static void *_ps_init_vp(void *arg)
{
	long vp = (long) arg;
	psthread_t *thread;
	psthread_attr_t ps_attr = PSTHREAD_ATTR_INITIALIZER;
	char *sp;
	uth_t me;
	
	me = uth_vp_init(vp,0);
        _bind_cpu(vp);

	/*
	 * Initialize primordial thread for this virtual processor
	 */
	
	thread = (psthread_t *) _ort_calloc_aligned(sizeof(psthread_t), NULL);
	if (!thread)
		return (void *) EAGAIN;
	
	thread->state = _PS_RUNNING;
	thread->nchildren = -1;
	thread->context = me;	/* uth_vp_init has already assigned an underlying thread (uth) to the current virtual processor  */
	uth_setarg(me, thread);	/* associate this user level thread with the corrersponding uth */
	_lock_init(&thread->lock);
	thread->vpid = vp;
	
	/*
	 * Create scheduler thread
	 */
	ps_attr.parent = thread;
	psthread_create(&_ps_scheduler_thread(vp), &ps_attr, _ps_scheduler_thread_start, (void *) vp);

	if (!_ps_scheduler_thread(vp))
		return NULL;
	
//	_ps_scheduler_thread(vp)->context = uth_create(_ps_thread_main, _ps_scheduler_thread(vp));
#if defined(UTH_SI_SB)
	sp = _ort_calloc_aligned_ex(_ps_stacksize, _ps_stacksize, NULL);
#else
	sp = _ort_calloc_aligned(_ps_stacksize, NULL);
#endif
//	printf("sp = %p\n", sp);
	_ps_scheduler_thread(vp)->context = uth_create_sp(_ps_thread_main, _ps_scheduler_thread(vp), _ps_stacksize, sp);

	
	pthread_mutex_lock(&gm);
	_ps_vp_created++;
	pthread_mutex_unlock(&gm);

	uth_switchto_ex(vp, NULL, _ps_scheduler_thread(vp)->context);
	
	/* Never returns here */
	return 0;
}


void psthread_stats()
{
	int i, j;
	int hit, att;
	double per;

	printf("======================\n");
	printf("SOFTWARE CONFIGURATION\n");
	printf("======================\n");
#if defined(LF_QUEUES)
	printf("LF_QUEUES\n");
#elif defined(LOCK_QUEUES)
	printf("LOCK_QUEUES\n");
#endif
#if defined(PS_P2P_HANDOFF)
	printf("PS_P2P_HANDOFF\n");
#endif
#if defined(PS_YIELDVP)
#if defined(PS_YIELDVP_EXP)
	printf("PS_YIELDVP:EXP\n");
#else
	printf("PS_YIELDVP:LIN\n");
#endif
#else
	printf("PS_NO_YIELDVP\n");
#endif
#if defined(PS_BAREX_YIELD)
	printf("PS_BAREX_YIELD\n");
#else
	printf("PS_BAREX_QUEUE\n");
#endif


#if defined(POSIX_MUTEX_LOCK)
	printf("POSIX_MUTEX_LOCK\n");
#elif defined(POSIX_MUTEX_TRYLOCK)
	printf("POSIX_MUTEX_TRYLOCK\n");
#elif defined(POSIX_SPIN_LOCK)
	printf("POSIX_SPIN_LOCK\n");
#elif defined(POSIX_SPIN_TRYLOCK)
	printf("POSIX_SPIN_TRYLOCK\n");
#endif

	printf("DEQUEUE METHODS: L:%c,R:%c,G:%c\n",
		_ps_ldq?'H':'E', _ps_rdq? 'H':'E', _ps_gdq? 'H':'E');	

	printf("BINDCPU: %s\n", _ps_bindcpu ? "ENABLED": "DISABLED");

	if (_ps_stealing_env == _PS_WORK_STEALING_DISABLED) {
		printf("WORK STEALING: DISABLED\n");
	}
	if (_ps_stealing_env == _PS_WORK_STEALING_ENABLED) {
		printf("WORK STEALING: ENABLED\n");
	}
	if (_ps_stealing_env == _PS_WORK_STEALING_DYNAMIC) {
		printf("WORK STEALING: DYNAMIC\n");
	}

	uth_self_identification_method();

#if defined(PS_STATS)
	for (i = 0; i < _ps_nvps; i++) {
		hit = _stl_hit[i][0];
		att = _stl_att[i][0];
		if (att == 0)	per = 0;
		else		per = (double) hit / (double) att;
		printf("vp:%2d att:%-5d hit:%-5d [%5.2lf%%]\n", i, att, hit, per*100.0);
	}

	for (i = 0; i < _ps_nvps; i++) {
		printf("%2d: ", i);
		for (j = 0; j < _ps_nvps; j++) {
			printf("%7d ", _stl_hit2[i][j][0]);
		}
		printf("| %7d ", _stl_hit2[i][j][0]);
		printf("\n");
	}

	printf("PSTHREAD DESCRIPTORS\n");
	for (i = 0; i < _ps_nvps; i++) {
		printf("vp %2d: created=%-5d reused=%-5d\n", i, _ps_thread_created(i), _ps_thread_reused(i));
	}
	uth_stats();
#endif
	return;
}

/*
 * Initializes the library structures, creates virtual processors and
 * scheduler threads.
 */
int psthread_init(int vps, unsigned long stacksize)
{
	psthread_t *thread;
	long i;
	pthread_t dummy;
	pthread_attr_t attr;
	psthread_attr_t ps_attr = PSTHREAD_ATTR_INITIALIZER;
	char *sp;
	uth_t me;

	printf("psthread_init(%d, %ld) called [%s]\n", vps, stacksize, PWD);
	
	_ps_gdata = (struct _ps_global_data *) _ort_calloc_aligned(sizeof(struct _ps_global_data), NULL);
	_ps_nvps = vps;
	_ps_vp_created=1;
	_ps_stacksize = stacksize;
#if 1	/* override */
	{
	char *s, ch;
	s = (char *) getenv("PSTHREADS_NVPS");
	if (s != 0 && sscanf(s, "%d", &i) == 1 && i > 0)
		_ps_nvps = i;

	printf("psthread_init: overriding for %d workers\n", _ps_nvps);

	s = (char *) getenv("PSTHREADS_STEALING");
	if (s != 0 && sscanf(s, "%d", &i) == 1 && i > 0) {
		printf("work stealing...[%d]\n", i);
		_ps_stealing_env = i;
		if ((_ps_stealing_env == _PS_WORK_STEALING_ENABLED) || (_ps_stealing_env == _PS_WORK_STEALING_DYNAMIC)) {
			printf("enabling work stealing...\n");
			_ps_stealing = 1;
		}
	}

	_ps_bindcpu = 0;
	s = (char *) getenv("PSTHREADS_BINDCPU");
	if (s != 0)
		_ps_bindcpu = 1;

	s = (char *) getenv("PSTHREADS_LDQ");
	if (s != 0 && sscanf(s, "%c", &ch) == 1 && i > 0) {
		if (ch == 'H' || ch == 'h')
			_ps_ldq=1;
		if (ch == 'E' || ch == 'e')
			_ps_ldq=0;
	}

	s = (char *) getenv("PSTHREADS_RDQ");
	if (s != 0 && sscanf(s, "%c", &ch) == 1 && i > 0) {
		if (ch == 'H' || ch == 'h')
			_ps_rdq=1;
		if (ch == 'E' || ch == 'e')
			_ps_rdq=0;
	}

	s = (char *) getenv("PSTHREADS_GDQ");
	if (s != 0 && sscanf(s, "%c", &ch) == 1 && i > 0) {
		if (ch == 'H' || ch == 'h')
			_ps_gdq=1;
		if (ch == 'E' || ch == 'e')
			_ps_gdq=0;
	}

	}
#endif
	
	_queue_init(&_ps_ready_queue);
	_queue_init(&_ps_reuse_queue);
	for (i = 0; i < _ps_nvps; i++) {
		_queue_init(&_ps_prive_ready_queue[i]);
		_queue_init(&_ps_local_ready_queue[i]);
		_queue_init_bare(&_ps_lreuse_queue[i]);
	}

	
	uth_init(_ps_stacksize);	/* Initialize uthlib */
	_ps_stacksize = uth_get_stacksize();
	me = uth_vp_init(0,0);	/* Initialize the first virtual processor (Posix Thread) */
        _bind_cpu(0);

	/*
	 * Initialize primordial thread for this virtual processor
	 */
	
	thread = (psthread_t *) _ort_calloc_aligned(sizeof(psthread_t), NULL);
	if (!thread)
		return EAGAIN;
	
	thread->state = _PS_RUNNING;
	thread->nchildren = -1;
	thread->context = me; //uth_self_ex(0);	/* uth_vp_init has already assigned an underlying thread (uth) to the current virtual processor  */
	uth_setarg(me, thread);	/* associate this user level thread with the corrersponding uth */
	_lock_init(&thread->lock);
	thread->vpid = 0;


	/*
	 * Create scheduler thread
	 */
	ps_attr.parent = thread;
	psthread_create(&_ps_scheduler_thread(0), &ps_attr, _ps_scheduler_thread_start, (void *) 0);

	if (!_ps_scheduler_thread(0))
		return EAGAIN;
	
//	_ps_scheduler_thread(0)->context = uth_create(_ps_thread_main, _ps_scheduler_thread(0));
#if defined(UTH_SI_SB)
	sp = _ort_calloc_aligned_ex(_ps_stacksize, _ps_stacksize, NULL);
#else
	sp = _ort_calloc_aligned(_ps_stacksize, NULL);
#endif
//	printf("sp = %p\n", sp);
	_ps_scheduler_thread(0)->context = uth_create_sp(_ps_thread_main, _ps_scheduler_thread(0), _ps_stacksize, sp);
	
	/* 
	 * Create the rest virtual processors 
	 */
	/*printf("_ps_nvps = %d\n", _ps_nvps);*/
	if (_ps_nvps > 1) {
		pthread_attr_init(&attr);
		if (pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM) != 0) {
			fprintf(stderr, "[PSTHREADS] warning: pthread_attr_setscope failed\n");
			fflush(stderr);
		}

		if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0) {
			fprintf(stderr, "[PSTHREADS] warning: pthread_attr_setdetachstate failed\n");
			fflush(stderr);
		}

		if (pthread_attr_setstacksize(&attr, 4*1024*1024) != 0) {
			fprintf(stderr, "[PSTHREADS] warning: pthread_attr_setstacksize failed\n");
			fflush(stderr);
		}
		
		for (i = 1; i < _ps_nvps; i++) {
			if (pthread_create(&dummy, &attr, _ps_init_vp, (void *)i) != 0) {
				fprintf(stderr, "[PSTHREADS] error: pthread_create failed\n");
				fflush(stderr);
				exit(1);
			}
		}

		pthread_mutex_lock(&gm);
		while (_ps_vp_created < _ps_nvps) {
			pthread_mutex_unlock(&gm);
			sched_yield();
			pthread_mutex_lock(&gm);
		}
		pthread_mutex_unlock(&gm);
		
	}

	return 0;
}

/* 
 * Exit function called by each user-level thread for normal termination.
 * It never returns.
 */
static inline void _psthread_exit(psthread_t *me)
{
	int vp = me->vpid;
	int res;

//	if (me->staticmem == 0xFABE) _lock_acquire(&me->lock);

	me->state = _PS_ZOMBIE;

	/* If I am joinable */
	if (me->detachstate == PSTHREAD_JOINABLE) {	
		_lock_acquire(&me->parent->lock);
		res =  (--me->parent->nchildren == 0) && (me->parent->state == _PS_WAITING);
		_lock_release(&me->parent->lock);
		/* If my parent waits and I am its last child, make it runnable */
		if (res) {
			me->parent->state = _PS_RUNNABLE;
			//_ps_enqueue_thread(vp) = me->parent;
			int pid = me->parent->vpid;

			_enqueue_head(&_ps_prive_ready_queue[pid], me->parent);

		}
	}

#if defined(PS_P2P_HANDOFF)
        {
		psthread_t *thread = NULL;
		_ps_unlock_thread(vp) = NULL;
		_ps_vp_schedule_stage1(vp);
		thread = _ps_enqueue_thread(vp);
		if (thread == NULL) {
			thread = _ps_vp_schedule_stage2(vp);
		}
		else {
			_ps_enqueue_thread(vp) = NULL;
		}

		if (thread != NULL) {
			if (thread->context == NULL) {
				thread->state = _PS_RUNNING;
				thread->vpid = me->vpid;
				thread->context = me->context;
				thread->cleanup = me;
				uth_reinit(thread->context, _ps_thread_main, thread);
				uth_switchto_ex(vp, NULL, thread->context);
                        }
                        else {
				thread->state = _PS_RUNNING;
				thread->vpid = vp;
				thread->cleanup = me;
				uth_switchto_ex(vp, NULL, thread->context);
			}
			return;
		}
	}

#endif

	_ps_unlock_thread(vp) = me;
	uth_switchto_ex(vp, NULL, _ps_scheduler_thread(vp)->context);
}

void psthread_exit()
{
	psthread_t *me = psthread_self();
	int vp = me->vpid;

	_psthread_exit(me);
}

/* 
 * Driver function. It is called by each newly created user-level thread.
 * It executes the user-provided work function and ensures proper termination
 */
static void _ps_thread_main(void *arg)
{
	psthread_t *me = arg;

	if (me->toq) {
		_enqueue_head /*tail */(&_ps_local_ready_queue[me->vpid], me->toq);
		me->toq = NULL;
	}

#if defined(PS_P2P_HANDOFF)
	_ps_recycle_cleanup(me, 0);
#endif	
	/* Run thread main */
	(*me->start)(me->arg);
	
	/* All done, time to go away */
	_psthread_exit(me);
}



/* 
 * Thread creation function: it allocates the thread structure
 * and creates the corresponding underlying thread.
 */
int psthread_create(psthread_t **t, psthread_attr_t *attr, void (*start)(void *arg), void *arg)	
{
	psthread_t *me;
	int res = 0;
	psthread_t *thread = NULL;

	if ((attr == NULL) || (attr->parent == NULL)) {
		me = psthread_self();
	}
	else {
		me = attr->parent;
	}

#if defined(UTH_REUSE)
#if defined(UTH_LOCAL_REUSE)
	_dequeue_bare(&_ps_lreuse_queue[me->vpid], &thread);
#else
	_dequeue(&_ps_reuse_queue, &thread);
#endif
	if (thread) _ps_thread_reused(me->vpid)++;
#endif
	if (thread == NULL) {
		thread = _ort_calloc_aligned(sizeof(psthread_t), NULL);	/* thread->realmem */
		if (thread) _ps_thread_created(me->vpid)++;
	}

	if (thread == NULL) {
		return EAGAIN;
	}

	/* Initialize thread */
	thread->start = start;
	thread->arg = arg;
	_lock_init(&thread->lock);
	thread->prev = thread->next = NULL;
	thread->vpid = -1;	
	thread->nchildren = 0;
	thread->detachstate= (attr == NULL)? 0: attr->detachstate;
	thread->staticmem = 0;
	thread->cleanup = NULL;
	thread->parent = me;
	memset(thread->tls, 0, MAX_TLS_KEYS*sizeof(void *));

	/* Any psthread is mapped to an underlying thread.
	 * It is executed through a driver routine that takes
	 * as argument the psthread itself
	 */
	thread->context = NULL;
	
	/* Make thread runnable */
	thread->state = _PS_NEW;
	
	if (thread->detachstate == PSTHREAD_JOINABLE) {
		/* Increase my children */
		_lock_acquire(&me->lock);
		me->nchildren++;
		_lock_release(&me->lock);
	}

	/* Store the new thread descriptor */
	*t = thread;


	return res;
}

/* 
 * Thread creation function: it checks if an already allocated thread structure
 * is provided by the user
 */

int psthread_get_tsize()
{
	return sizeof(psthread_t);
}

void psthread_set_fabe(psthread_t *t)
{
	t->staticmem = 0xFABE;
	t->state = _PS_FREED;
	_lock_init(&t->lock);
}

char *psthread_alloc_desc(int n)
{
	psthread_t *t;
	int i;

	t = calloc(1, n*sizeof(psthread_t));
	for (i = 0; i < n; i++) {
		t[i].staticmem = 0xFABE;
		t[i].state = _PS_FREED;
		_lock_init(&t[i].lock);
	}
	return (char *)t;
}

int psthread_create_ex(psthread_t **t, psthread_attr_t *attr, void (*start)(void *arg), void *arg)	
{
	psthread_t *me;
	int res = 0;
	psthread_t *thread = NULL;

	if ((attr == NULL) || (attr->parent == NULL)) {
		me = psthread_self();
	}
	else {
		me = attr->parent;
	}

	if (*t != NULL) {
		thread = *t;
		if (thread->staticmem == 0xFABE) {
			unsigned long x = 1;
			/* one solution: pure lock - unlock */
			/* second solution: check thread state */
			_lock_acquire(&thread->lock);
			while (thread->state != _PS_FREED) {
				_lock_release(&thread->lock);
				x++;
				//if (x % 10 == 0) sched_yield();
				//psthread_yield();
				_lock_acquire(&thread->lock);
				//if (x % 1000 == 0) printf("!\n");
			}
			_lock_release(&thread->lock);
		
			//if (x > 1) printf("x = %ld\n", x);
			goto label100;
		} else {
			printf("warning: invalid thread descriptor in psthread_create_ex, switching to psthread_create\n");
			thread = NULL;
		}
	}

#if defined(UTH_REUSE)
#if defined(UTH_LOCAL_REUSE)
	_dequeue_bare(&_ps_lreuse_queue[me->vpid], &thread);
#else
	_dequeue(&_ps_reuse_queue, &thread);
#endif
#endif
	if (thread == NULL) {
		thread = _ort_calloc_aligned(sizeof(psthread_t), NULL);
	}

	if (thread == NULL) {
		return EAGAIN;
	}

	/* moved some of initialization here */
//label100:
	_lock_init(&thread->lock);
	thread->prev = thread->next = NULL;
	thread->nchildren = 0;
	thread->cleanup = NULL;
//	thread->context = NULL;
	thread->staticmem = 0xFABE;
	memset(thread->tls, 0, MAX_TLS_KEYS*sizeof(void *));	//???
label100:
	/* Initialize thread */
	thread->context = NULL;
	thread->start = start;
	thread->arg = arg;
	thread->vpid = -1;	
	thread->detachstate= (attr == NULL)? 0: attr->detachstate;
	thread->parent = me;

	/* Make thread runnable */
	thread->state = _PS_NEW;
	
	if (thread->detachstate == PSTHREAD_JOINABLE) {
		/* Increase my children */
		_lock_acquire(&me->lock);
		me->nchildren++;
		_lock_release(&me->lock);
	}

	/* Store the new thread descriptor */
	*t = thread;


	return res;
}


/* 
 * Parent thread waits for all its children to terminate 
 */
int psthread_waitall()
{
	psthread_t *me = psthread_self();
	int vp = me->vpid;

	_lock_acquire(&me->lock);
	if (me->nchildren == 0) {
		_lock_release(&me->lock);
		return 1;
	}
	me->state = _PS_WAITING;
	_ps_unlock_thread(vp) = me;
	uth_switchto_ex(vp, me->context, _ps_scheduler_thread(vp)->context);
#if defined(PS_P2P_HANDOFF)
	_ps_recycle_cleanup(me, 1);
#endif
	return 0;
}

/*
 * Queue management routines
 */
int psthread_enqueue_tail_prv(psthread_t *thread, int queue)
{
	if ((queue < 0) && (queue >= _ps_nvps))
		return EINVAL;

	thread->state = _PS_RUNNABLE;
	_enqueue_tail(&_ps_prive_ready_queue[queue], thread);

	return 0;
}

int psthread_enqueue_tail(psthread_t *thread, int queue)
{
	if ((queue < -1) && (queue >= _ps_nvps))
		return EINVAL;

	thread->state = _PS_RUNNABLE;
#if defined(PS_GLOBAL_QUEUE_ONLY)
	queue = -1;
#endif
	if (queue == -1) {
		_enqueue_tail(&_ps_ready_queue, thread);
	} 
	else {
		_enqueue_tail(&_ps_local_ready_queue[queue], thread);
	}

	return 0;
}

int psthread_enqueue_bunch_tail(psthread_t **thread, int n, int queue)
{
	int i;
	if ((queue < -1) && (queue >= _ps_nvps))
		return EINVAL;

	for (i = 0; i < n; i++) {
//		thread[i]->prev = thread[i]->next = NULL;
		thread[i]->state = _PS_RUNNABLE;
	}
	for (i = 1; i < n; i++) {
		thread[i-1]->next = thread[i];
		thread[i]->prev = thread[i-1];
	}

#if defined(PS_GLOBAL_QUEUE_ONLY)
	queue = -1;
#endif
#if 0
	if (queue == -1) {
		_enqueue_bunch_tail(&_ps_ready_queue, thread[0], thread[n-1]);
	} 
	else {
		_enqueue_bunch_tail(&_ps_local_ready_queue[queue], thread[0], thread[n-1]);
	}
#endif
	return 0;
}

int psthread_enqueue_head(psthread_t *thread, int queue)
{
	if ((queue < -1) && (queue >= _ps_nvps))
		return EINVAL;

	thread->state = _PS_RUNNABLE;
#if defined(PS_GLOBAL_QUEUE_ONLY)
	queue = -1;
#endif

	if (queue == -1) {
		_enqueue_head(&_ps_ready_queue, thread);
	} 
	else {
		_enqueue_head(&_ps_local_ready_queue[queue], thread);
	}

	return 0;
}

int psthread_enqueue(psthread_t *thread, int queue)
{
	return psthread_enqueue_tail(thread, queue);
}


/* 
 * Return a pointer to the descriptor of the calling thread
 */ 
extern void *_uth_self_from_stack();

psthread_t *psthread_self(void)
{
	/* The descriptor is the argument of the corresponding underlying thread */
	return uth_getarg(uth_self());
}

psthread_t *psthread_self_ex(int vp)
{
	/* The descriptor is the argument of the corresponding underlying thread */
	return uth_getarg_ex(vp);
}


/* 
 * Return elapsed time in sec
 */
double psthread_gettime()
{
	return uth_gettime();
}

/* 
 * Suspend virtual processor for ms milliseconds
 */
void psthread_vp_sleep(int ms)
{
	uth_vp_sleep(ms);
}


/*
 * Virtual processors info
 */
int psthread_nvps(void)
{
	return _ps_nvps;
}

int psthread_current_vp(void)
{
	return uth_get_vpid_tsd();
}

unsigned long psthread_vp_self()
{
	return uth_vp_self();
}


/*
 * Condition variables
 */

void psthread_cond_init(psthread_cond_t *cond)
{
	_queue_init(cond);
}

void psthread_cond_destroy (psthread_cond_t *cond)
{
	_queue_destroy(cond);
}

void psthread_cond_wait_ex (psthread_t *me, psthread_cond_t *cond, psthread_lock_t *lock)
{
//	psthread_t *me;
	int vp;

	if (me == NULL) me = psthread_self();
	vp = me->vpid;

	if (lock != NULL) {
		_lock_acquire(&me->lock);
	}

	_enqueue_tail(cond, me);

	if (lock != NULL) {
		_lock_release(lock);
	}

	me->state = _PS_WAITING;
	_ps_unlock_thread(vp) = me;

	uth_switchto_ex(vp, me->context, _ps_scheduler_thread(vp)->context);
#if defined(PS_P2P_HANDOFF)
	_ps_recycle_cleanup(me, 1);
#endif
	if (lock != NULL) {				/* for barrier */
		_lock_acquire(lock);
	}
}

void psthread_cond_wait (psthread_cond_t *cond, psthread_lock_t *lock)
{
	psthread_t *me = psthread_self();
	int vp = me->vpid;

	if (lock != NULL) {
		_lock_acquire(&me->lock);
	}

	_enqueue_tail(cond, me);

	if (lock != NULL) {
		_lock_release(lock);
	}

	me->state = _PS_WAITING;
	_ps_unlock_thread(vp) = me;

	uth_switchto_ex(vp, me->context, _ps_scheduler_thread(vp)->context);
#if defined(PS_P2P_HANDOFF)
	_ps_recycle_cleanup(me, 1);
#endif
	if (lock != NULL) {				/* for barrier */
		_lock_acquire(lock);
	}
}

void psthread_cond_signal (psthread_cond_t *cond)
{
	psthread_t *thread = NULL;

	_dequeue(cond, &thread);
	if (thread != NULL) {
		_lock_acquire(&thread->lock);
		thread->prev = NULL;
		thread->next = NULL;
		thread->state = _PS_RUNNABLE;
		_lock_release(&thread->lock);
#if defined(PS_GLOBAL_QUEUE_ONLY)
		_enqueue_head(&_ps_ready_queue, thread);
#else
		_enqueue_head(&_ps_local_ready_queue[thread->vpid], thread);
#endif
	}
	else {
		_queue_init(cond);	/* is it necessary ? */
	}
}

void psthread_cond_signal_ex (psthread_cond_t *cond)
{
	psthread_t *thread = NULL;

	_dequeue_bare(cond, &thread);
	if (thread != NULL) {
		_lock_acquire(&thread->lock);
		thread->prev = NULL;
		thread->next = NULL;
		thread->state = _PS_RUNNABLE;
		_lock_release(&thread->lock);
#if defined(PS_GLOBAL_QUEUE_ONLY)
		_enqueue_head(&_ps_ready_queue, thread);
#else
		_enqueue_head(&_ps_local_ready_queue[thread->vpid], thread);
#endif
	}
	else {
//		_queue_init(cond);	/* is it necessary ? */
	}
}

void psthread_cond_broadcast (psthread_cond_t *cond)
{
	psthread_t *thread, *next;

	thread = _queue_head(cond);
	_queue_init(cond);
	while (thread != NULL) {
		_lock_acquire(&thread->lock);
		next = thread->next;
		thread->prev = thread->next = NULL;
		thread->state = _PS_RUNNABLE;
		_lock_release(&thread->lock);
#if defined(PS_GLOBAL_QUEUE_ONLY)
		_enqueue_head(&_ps_ready_queue, thread);
#else
		_enqueue_head(&_ps_local_ready_queue[thread->vpid], thread);
#endif
		thread = next;
	}
}


/*
 * Thread Local Storage (not fully functional)
 */
 
static int tls_keys[MAX_TLS_KEYS] = {0};
static pthread_mutex_t tls_mutex = PTHREAD_MUTEX_INITIALIZER;

int psthread_key_create(psthread_key_t *key, void *destructor)
{
	int i;

	pthread_mutex_lock(&tls_mutex);
	for (i = 0; i < MAX_TLS_KEYS; i++) {
		if (tls_keys[i] == 0) {
			tls_keys[i] = 1;
			*key = i;
			pthread_mutex_unlock(&tls_mutex);
			return 0;
		}
	}
	pthread_mutex_unlock(&tls_mutex);
	*key = -1;
	return 1;
}

int psthread_setspecific(psthread_key_t key, const void *value)
{
	psthread_t *me = psthread_self();
	int vp = me->vpid;

	if ((key < 0) || (key >= MAX_TLS_KEYS)) {
		return 1;
	}
	me->tls[key] = value;
	return 0;
}

void *psthread_getspecific(psthread_key_t key)
{
	psthread_t *me = psthread_self();

	if ((key < 0) || (key >= MAX_TLS_KEYS)) {
		return NULL;
	}
	return (void *)me->tls[key];
}

int psthread_key_delete(psthread_key_t key)
{
	if ((key < 0) || (key >= MAX_TLS_KEYS)) {
		return 1;
	}
	pthread_mutex_lock(&tls_mutex);
	tls_keys[key] = 0;
	pthread_mutex_unlock(&tls_mutex);

	return 0;
}


int psthread_key_clone(psthread_t *t1, psthread_t *t2)
{
	int i;
	pthread_mutex_lock(&tls_mutex);
	for (i = 0; i < MAX_TLS_KEYS; i++) {
		t2->tls[i] = t1->tls[i];
	}
	pthread_mutex_unlock(&tls_mutex);
	return 0;
}

void *psthread_getspecific_ex(psthread_t *t1, psthread_key_t key)
{
	return (void *) t1->tls[key];
}

int psthread_setspecific_ex(psthread_t *t1, psthread_key_t key, const void *value)
{
	psthread_t *t = t1;

	if ((key < 0) || (key >= MAX_TLS_KEYS)) {
		return 1;
	}
	t->tls[key] = value;
	return 0;
}


/*
 * Posix-like barriers (D. Butenhof)
 */

/*
 * Initialize a barrier for use.
 */
int psthread_barrier_init (psthread_barrier_t *barrier, int count)
{
	if (barrier->valid != _PS_BARRIER_VALID) {
		_lock_init (&barrier->mutex);
		psthread_cond_init (&barrier->cv);	/* _queue_init(&barrier->cv); */
		barrier->valid = _PS_BARRIER_VALID;
	}
	barrier->threshold = barrier->counter = count;
	barrier->cycle = 0;
//	barrier->aux = NULL;
//	barrier->ppf = NULL;
	return 0;
}



/*
 * Destroy a barrier when done using it.
 */
int psthread_barrier_destroy (psthread_barrier_t *barrier)
{
	if (barrier->valid != _PS_BARRIER_VALID)
		return EINVAL;

	_lock_acquire(&barrier->mutex);

	/*
	 * Check whether any threads are known to be waiting; report	
	 * "BUSY" if so.
	 */
	if (barrier->counter != barrier->threshold) {
		_lock_release (&barrier->mutex);
		return EBUSY;
	}

	barrier->valid = 0;
	_lock_release(&barrier->mutex);

	/*
	 * If unable to destroy either 1003.1c synchronization
	 * object, return the error status.
	 */

	_lock_destroy (&barrier->mutex);
	psthread_cond_destroy (&barrier->cv);	/* _queue_destroy(&barrier->cv); */

	return 0;
}

/*
 * Wait for all members of a barrier to reach the barrier. When
 * the count (of remaining members) reaches 0, broadcast to wake
 * all threads waiting.
 */
int psthread_barrier_wait (psthread_barrier_t *barrier)
{
	int status;
	unsigned long cycle;

	if (barrier->valid != _PS_BARRIER_VALID)
		return EINVAL;

	_lock_acquire(&barrier->mutex);

	cycle = barrier->cycle;   /* Remember which cycle we're on */

	if (--barrier->counter == 0) {

		barrier->cycle++;
		barrier->counter = barrier->threshold;

		psthread_cond_broadcast (&barrier->cv);

		/*
		 * The last thread into the barrier will return status
		 * -1 rather than 0, so that it can be used to perform
		 * some special serial code following the barrier.
		 */
		status = -1;
	} 
	else {
		/*
		 * Wait until the barrier's cycle changes, which means
		 * that it has been broadcast, and we don't want to wait
		 * anymore.
		 */
		while (cycle == barrier->cycle) {
			psthread_cond_wait (&barrier->cv, &barrier->mutex);
		}
		status = 0;
	}

	_lock_release(&barrier->mutex);

	return status;          /* error, -1 for waker, or 0 */
}


/*
 * Fast Psthreads barriers (phadjido)
 */

int psthread_barrier_init_ex (psthread_barrier_ex_t *barrier, int count)
{
	if (barrier->valid != _PS_BARRIER_VALID) {
		_lock_init (&barrier->mutex);
#if defined(PS_BAREX_QUEUE)
		barrier->threads = calloc(1, count*sizeof(psthread_t *));	//
#endif
#if defined(PS_BAREX_YIELD)
		barrier->arrived = calloc(1, count*sizeof(ps_aligned_int));
#endif
		barrier->valid = _PS_BARRIER_VALID;
	}

	barrier->threshold = barrier->counter = count;
	barrier->aux = NULL;
	barrier->ppf = NULL;
#if defined(PS_BAREX_YIELD)
	barrier->sense = 0;
	barrier->nthr = count;
	for (--count;  count >= 0; count--)
		barrier->arrived[count].value = 0;    
#endif

	return 0;
}

/*
 * Destroy a barrier when done using it.
 */
int psthread_barrier_destroy_ex (psthread_barrier_ex_t *barrier)
{
	if (barrier->valid != _PS_BARRIER_VALID)
		return EINVAL;

	_lock_acquire(&barrier->mutex);
	barrier->valid = 0;
	_lock_release(&barrier->mutex);

	/*
	 * If unable to destroy either 1003.1c synchronization
	 * object, return the error status.
	 */

	_lock_destroy (&barrier->mutex);
	free(barrier->threads);
	free(barrier->arrived);
	return 0;
}

/*
 * Wait for all members of a barrier to reach the barrier. When
 * the count (of remaining members) reaches 0, broadcast to wake
 * all threads waiting.
 */
int psthread_barrier_wait_ex (psthread_barrier_ex_t *barrier, int id)
{
#if defined(PS_BAREX_YIELD)
	int sense;
	int status = -(id == 0);

	if (barrier->nthr == 1) return status;
	sense = !( barrier->sense );
	if (id > 0) 
	{
		barrier->arrived[id].value = sense;
		FENCE;
		PS_WAIT_WHILE( barrier->sense != sense, YIELD_FREQUENTLY );
	}
	else     /* Let the master do the work */
	{
		for (id = 1; id < barrier->nthr; id++)
			PS_WAIT_WHILE( barrier->arrived[id].value != sense, YIELD_IMMEDIATELY );
		barrier->sense = sense;      /* Release them all */                
		FENCE;
	}
	return status;
#endif
#if defined(PS_BAREX_QUEUE)
	int status = 0;
	unsigned long cycle;
	psthread_t *me = psthread_self();
	int i;
	int vp = me->vpid;
	psthread_t *thread;
	int res;

	//printf("thread %d entering...\n", id);

	if (barrier->valid != _PS_BARRIER_VALID)
		return EINVAL;

	if (_ps_stealing_env == _PS_WORK_STEALING_DYNAMIC) _ps_stealing = 0;	/* if me->level == 1 */


	_lock_acquire(&me->lock);

	_lock_acquire(&barrier->mutex);
	barrier->threads[id]=me;
	res = --barrier->counter;
	_lock_release(&barrier->mutex);

	if (res == 0) {
		_lock_acquire(&barrier->mutex);
		if ((barrier->ppf)&&(barrier->aux)) barrier->ppf(barrier->aux, barrier->threshold);
		for (i=0; i<barrier->threshold;i++) {
			if (i == id) continue;
			thread = barrier->threads[i];
			_lock_acquire(&thread->lock);
			thread->prev = NULL;
			thread->next = NULL;
			thread->state = _PS_RUNNABLE;
			_lock_release(&thread->lock);
#if defined(PS_GLOBAL_QUEUE_ONLY)
			_enqueue_head(&_ps_ready_queue, thread);
#else
//			printf("%d->%d\n", i, thread->vpid);
//			_enqueue_head(&_ps_local_ready_queue[thread->vpid], thread);	// this goes with dynamic stealing
			_enqueue_head(&_ps_prive_ready_queue[thread->vpid], thread);
#endif
		}
		_lock_release(&me->lock);
		if (_ps_stealing_env == _PS_WORK_STEALING_DYNAMIC) _ps_stealing = 1;
		barrier->counter = barrier->threshold;
		_lock_release(&barrier->mutex);
		status = -1;
	}
	else {
		me->state = _PS_WAITING;
		_ps_unlock_thread(vp) = me;

		uth_switchto_ex(vp, me->context, _ps_scheduler_thread(vp)->context);
#if defined(PS_P2P_HANDOFF)
		_ps_recycle_cleanup(me, 1);
#endif
		status = 0;
	}

	return status;          /* error, -1 for waker, or 0 */
#endif
}


void psthread_version()
{
	printf(">>>> This is %s library %s\n", PACKAGE_NAME, PACKAGE_VERSION);
	uth_info();
}
