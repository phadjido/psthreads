/* 
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is the Netscape Portable Runtime library.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 1994-2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):  Silicon Graphics, Inc.
 * 
 * Portions created by SGI are Copyright (C) 2000-2001 Silicon
 * Graphics, Inc.  All Rights Reserved.
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */

/*
 * This file is derived directly from the State Threads library, 
 * modified extensively by P.E. Hadjidoukas for PSthreads/UthLib. 
 */

#include "uth_common.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#if !defined(WIN32_MSVC) && !defined(MINGW)
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#else
/*#define _WIN32_WINNT	0x0500*/
#include <windows.h>
#endif
#include <string.h>
#include <math.h>
/* Internal global data - defined in common.h */

struct uth_data			*_uth_data;
uth_stats_t			_uth_stats;

pthread_key_t			_uth_vp_key;
pthread_key_t			_uth_curr_key;
pthread_mutex_t			_uth_mutex = PTHREAD_MUTEX_INITIALIZER;

int 				_uth_max_vp = -1;
unsigned int			ssizebits;
uth_t 				*main_uth = NULL;
unsigned long			main_sp = 0x0;

#if defined(UTH_SI_SB)
#define _uth_get_thread_vp(thread, vp)			\
	{						\
	thread = _uth_self_from_stack();		\
	vp = thread->vp_id;				\
	}
#elif defined(UTH_SI_TSD)
#define _uth_get_thread_vp(thread, vp)			\
	{						\
	thread = pthread_getspecific(_uth_curr_key);	\
	vp = thread->vp_id;				\
	}
#elif defined(UTH_SI_VPID)
#define _uth_get_thread_vp(thread, vp)			\
	{						\
	vp = _uth_get_vpid_tsd();			\
	thread =  _UTH_CURRENT_THREAD(vp);		\
	}
#else
#error "Self-identification method not defined"
#endif


/*
 * Queue management for thread recycling
 */

/* Queue Initialization */
void _uth_queue_init (queue_t * q)
{
	_queue_init(q);
}

/* Insert a thread into the global reuse queue */
void _uth_to_reuseq (uth_t *uth)
{
	_enqueue_head(&reuse_uth_q, uth);
}

/* Extract a thread from the global reuse queue */
uth_t *_uth_reuseq_dequeue ()
{
	uth_t *uth = NULL;
    
	_dequeue(&reuse_uth_q, &uth);
	return uth;
}

/* Insert a thread into a local reuse queue */
void _uth_to_reuselrq (int qid, register uth_t *uth)
{
#if !defined(UTH_LOCKED_REUSE)
	_enqueue_head_bare(&reuse_uth_lq[qid], uth);
#else
	_enqueue_head(&reuse_uth_lq[qid], uth);
#endif
}

/* Extract a thread from a local reuse queue */
uth_t *_uth_reuselrq_dequeue (int qid)
{
	uth_t *uth = NULL;
    
#if !defined(UTH_LOCKED_REUSE)
	_dequeue_bare(&reuse_uth_lq[qid], &uth);
#else
	_dequeue(&reuse_uth_lq[qid], &uth);
#endif
	return uth;
}

/* General routine: returns a recycled thread */
uth_t *_uth_get_reused_thread(void)
{
	uth_t *thread;

#if defined(UTH_LOCAL_REUSE)
	int vp = uth_get_vpid_tsd();
	thread = _uth_reuselrq_dequeue (vp);
//	if (thread) stacks_queued(vp)--;
//	if (thread == NULL) thread = _uth_reuseq_dequeue();
#else
	thread = _uth_reuseq_dequeue();
#endif

	return thread;
}

/* General routine: recycles a finished thread */
void _uth_put_reused_thread(uth_t *thread)
{
#if defined(UTH_LOCAL_REUSE)
	int vp;
#if !defined(UTH_LOCKED_REUSE)
//	vp = uth_get_vpid_tsd();
	vp = thread->vp_id;
#else
	vp = thread->vp_cr;
#endif
//	printf("sq(%d)=%d\n", vp, stacks_queued(vp));
//	vp = rand()% _uth_max_vp;
//	stacks_queued(vp)++;
	_uth_to_reuselrq(vp, thread);
#else
	_uth_to_reuseq(thread);
#endif
}

/* 
 * Get system page size (syscall not available on Win32 and QNX platforms)
 */ 

#if defined(WIN32_MSVC)
size_t getpagesize(void)
{
	SYSTEM_INFO sysinf;
	GetSystemInfo(&sysinf);
	return (sysinf.dwPageSize);
}
#elif defined(QNX)
size_t getpagesize(void)
{
	return 4096;
}
#endif

void _uth_data_init ()
{
	char *mem;

	mem = _ort_calloc_aligned(sizeof (struct uth_data), NULL);
	if (mem == NULL) {
		fprintf (stderr, "Allocating uth_data (size %ld): malloc failed",
				(unsigned long)sizeof (struct uth_data));
		exit (1);
	}

	_uth_data = (void *) mem;
}

void print_uth_data ()
{
	int i;
	printf ("print_uth_data          0x%lx\n", (unsigned long) print_uth_data);

	printf ("uth_data at             0x%lx\n", (unsigned long) _uth_data);
	printf ("sizeof (uth_data)       0x%lx (%ld)\n", (unsigned long) sizeof(_uth_data), (unsigned long)sizeof(_uth_data));

	printf (" reuse_q                0x%lx\n", (unsigned long) &reuse_uth_q);
	printf (" reuse_lq               0x%lx\n", (unsigned long) &reuse_uth_lq);
	i = 0;
	while (i<MAX_VPS) {
		printf (" reuse_lq[%2d]          0x%lx\n", i, (unsigned long) &reuse_uth_lq[i]);
		++i;
	}

	printf (" _UTH_CURRENT_THREAD    0x%lx\n", (unsigned long) &_UTH_CURRENT_THREAD(0));
	printf (" _UTH_VP_IDS            0x%lx\n", (unsigned long) &vp_ids);

	printf (" _UTH_PAGE_SIZE         0x%lx\n", (unsigned long) &_UTH_PAGE_SIZE);
	printf (" _UTH_STACK_SIZE        0x%lx\n", (unsigned long) &_UTH_STACK_SIZE);
}

/* General library initialization */
static int initialized = 0;

int uth_init(long stacksize)
{
	int i;

	if (initialized)
		return 0;
 
	_uth_data_init();
	if ((unsigned long) _uth_data & (CACHE_LINE_SIZE-1)) {
/*		fprintf (stderr, "WARNING: uth_data structure not cacheline-aligned (0x%lx)\n",
				(unsigned long) _uth_data);
*/
	}
#if 0
	print_uth_data();
#endif
	_uth_queue_init(&reuse_uth_q);
	for (i = 0; i < MAX_VPS; i++) {
		_uth_queue_init(&reuse_uth_lq[i]);
	}

	_UTH_PAGE_SIZE = sysconf(_SC_PAGESIZE);
	if (stacksize == 0)
		_UTH_STACK_SIZE = UTH_DEFAULT_STACK_SIZE;
	else
		_UTH_STACK_SIZE = stacksize;

	memset(&_uth_stats, 0, sizeof(uth_stats_t));

	pthread_key_create(&_uth_vp_key, NULL);
	pthread_key_create(&_uth_curr_key, NULL);
//	pthread_spin_init(&ll, 0);

#if 0
	ssizebits = ilogbl(_UTH_STACK_SIZE);
#else
	{
		int x = _UTH_STACK_SIZE;
		ssizebits = 0;
		while ((x/=2) != 0) ssizebits++;
	}
#endif

	initialized = 1;
	return 0;
}

/* 
 * Initialize the current virtual processor (Posix Thread) and assign
 * an identifier (vpid) to it
 */
uth_t *uth_vp_init(int vpid, void *arg)
{
	uth_t *thread;
	int i;

	if (!initialized) {
		_uth_data_init();
		_uth_queue_init(&reuse_uth_q);
		for (i = 0; i < MAX_VPS; i++) {
			_uth_queue_init(&reuse_uth_lq[i]);
		}
		_UTH_PAGE_SIZE = sysconf(_SC_PAGESIZE);
		_UTH_STACK_SIZE = UTH_DEFAULT_STACK_SIZE;

		memset(&_uth_stats, 0, sizeof(uth_stats_t));

		initialized = 1;
	}

	pthread_mutex_lock(&_uth_mutex);
	if (vpid > _uth_max_vp) _uth_max_vp = vpid;
	pthread_mutex_unlock(&_uth_mutex);


	vp_ids[vpid][0] = pthread_self();	/* store my pthread_t - used for self identification */


	thread = (uth_t *) calloc(1, sizeof(uth_t));	/* associate a user level thread with this Posix Thread */
	if (!thread)
		return (void *)-1;

	thread->vp_id = vpid;
	thread->arg = NULL;
        thread->retval = (void *) 0xDEAD;
	pthread_setspecific(_uth_vp_key, (void *)(intptr_t)vpid);
#if defined(UTH_SI_TSD)
	pthread_setspecific(_uth_curr_key, thread);
#endif








	_UTH_SET_CURRENT_THREAD(vpid, thread);

	if (vpid == 0) {
		main_sp = (unsigned long) &i;
		main_uth = thread;
	}

	return thread;
}

/* 
 * Warning: if we use stack-based self identification,
 * this function must be called only once, during the initialiation,
 * (just after uth_init(0)) 
 */
int uth_set_stacksize(int stacksize)
{
	int previous = _UTH_STACK_SIZE;
	_UTH_STACK_SIZE = stacksize;
	if (stacksize == 0) _UTH_STACK_SIZE = UTH_DEFAULT_STACK_SIZE;

	return previous;
}

int uth_get_stacksize()           
{
        return _UTH_STACK_SIZE;
}

/* Driver routine */
void _uth_main(void)
{ 
	uth_t *thread;
	int vp;

	_uth_get_thread_vp(thread, vp);
//	printf("[%p] uth=%p, vp=%d\n", pthread_self(), thread, vp);

	/* Run thread main */
	(*thread->start)(thread->arg);
	/* I can put a cleanup routine here */
}

/* 
 * Creates an (underlying) thread. The thread descriptor
 * is allocated in the stack of the thread
 */
uth_t *uth_create(void (*start)(void *arg), void *arg)
{	int vp;
	uth_t *thread;
	uth_stack_t *stack;
	uth_stack_t mystack;
	int stk_size;
	char *sp;
#ifdef __ia64__
	char *bsp;
#endif
	stk_size = _UTH_STACK_SIZE;	/* could be function argument */
#if defined(UTH_REUSE)
	/* Try to reuse a finished thread */
	if ((thread = _uth_get_reused_thread()) == NULL) {
#endif
		/* Thread/stack allocation and initialization */
#if defined(UTH_STATS)
		vp = uth_get_vpid_tsd();
		stacks_created(vp)++;
#endif
		/* Adjust stack size */
		stk_size = ((stk_size + _UTH_PAGE_SIZE - 1) / _UTH_PAGE_SIZE) * _UTH_PAGE_SIZE;
		stack = _uth_stack_new(&mystack, stk_size);
		if (!stack)
			return NULL;

		/* Allocate thread object and per-thread data off the stack */
#if defined(MD_STACK_GROWS_DOWN)
		sp = stack->stk_top;
#ifdef __ia64__
		/*
		 * The stack segment is split in the middle. The upper half is used
		 * as backing store for the register stack which grows upward.
		 * The lower half is used for the traditional memory stack which
		 * grows downward. Both stacks start in the middle and grow outward
		 * from each other.
		 */
		sp -= (stk_size >> 1);
		bsp = sp;
		/* Make register stack 64-byte aligned */
		if ((unsigned long)bsp & 0x3f)
			bsp = bsp + (0x40 - ((unsigned long)bsp & 0x3f));
		stack->bsp = bsp + _UTH_STACK_PAD_SIZE;
#endif
		/* thread descriptor is stored in its stack */
		sp = sp - sizeof(uth_t);
		thread = (uth_t *) sp;

#if !defined(UTH_PAGE_ALIGNED_STACK)
		/* Make stack 64-byte aligned */
		if ((unsigned long)sp & 0x3f)
			sp = sp - ((unsigned long)sp & 0x3f);
		stack->sp = sp - _UTH_STACK_PAD_SIZE;
#else
		/* Make stack page aligned */
		if ((unsigned long)sp & (_UTH_PAGE_SIZE-1))
			sp = sp - ((unsigned long)sp & (_UTH_PAGE_SIZE-1));
		stack->sp = sp;	/*- _UTH_STACK_PAD_SIZE;*/
#endif
#elif defined(MD_STACK_GROWS_UP)
		sp = stack->stk_bottom;
		thread = (uth_t *) sp;
		sp = sp + sizeof(uth_t);

#ifndef PAGE_ALIGNED_STACK
		/* Make stack 64-byte aligned */
		if ((unsigned long)sp & 0x3f)
			sp = sp + (0x40 - ((unsigned long)sp & 0x3f));
		stack->sp = sp + _UTH_STACK_PAD_SIZE;
#else
		/* Make stack page-byte aligned (sparc -> 8192!!)*/
		if ((unsigned long)sp & (_UTH_PAGE_SIZE-1))
			sp = sp + ((_UTH_PAGE_SIZE) - ((unsigned long)sp & (_UTH_PAGE_SIZE-1)));
		stack->sp = sp; /*+ _UTH_STACK_PAD_SIZE;*/
#endif
#else
#error Unknown OS
#endif
		memset(thread, 0, sizeof(uth_t));

		/* Initialize thread */
		memcpy(&thread->stackinfo, stack, sizeof(uth_stack_t));
		thread->stack = &thread->stackinfo;
		thread->start = start;
		thread->arg = arg;
#if defined(UTH_LOCKED_REUSE)
#if !defined(UTH_STATS)
		vp = uth_get_vpid_tsd();
#endif
		thread->vp_cr = vp;
#endif
		thread->retval = (void *)0xDEAD;







#ifndef __ia64__
		_UTH_INIT_CONTEXT(thread, stack->sp, _uth_main);
#else
		_UTH_INIT_CONTEXT(thread, stack->sp, stack->bsp, _uth_main);
#endif

#if defined(UTH_REUSE)
	}
	else {
#if defined(UTH_STATS)
		int vp = uth_get_vpid_tsd();
		stacks_reused(vp)++;
#endif
		/* Recycled thread: just reinitialize it */
		uth_reinit(thread, start, arg);
	}
#endif

	return thread;
}


uth_t *uth_create_sp(void (*start)(void *arg), void *arg, int stacksize, char *vaddr)
{
	uth_t *thread;
	uth_stack_t *stack;
	uth_stack_t mystack;
	int stk_size;
	char *sp;
#ifdef __ia64__
	char *bsp;
#endif
	stk_size = stacksize;
	/* Thread/stack allocation and initialization */

	/* Adjust stack size */
	//stk_size = ((stk_size + _UTH_PAGE_SIZE - 1) / _UTH_PAGE_SIZE) * _UTH_PAGE_SIZE;
	stack = _uth_stack_new_sp(&mystack, stk_size, vaddr);
	if (!stack)
		return NULL;

	/* Allocate thread object and per-thread data off the stack */
#if defined(MD_STACK_GROWS_DOWN)
	sp = stack->stk_top;
#ifdef __ia64__
	/*
	 * The stack segment is split in the middle. The upper half is used
	 * as backing store for the register stack which grows upward.
	 * The lower half is used for the traditional memory stack which
	 * grows downward. Both stacks start in the middle and grow outward
	 * from each other.
	 */
	sp -= (stk_size >> 1);
	bsp = sp;
	/* Make register stack 64-byte aligned */
	if ((unsigned long)bsp & 0x3f)
		bsp = bsp + (0x40 - ((unsigned long)bsp & 0x3f));
	stack->bsp = bsp + _UTH_STACK_PAD_SIZE;
#endif
	/* thread descriptor is stored in its stack */
	sp = sp - sizeof(uth_t);
	thread = (uth_t *) sp;

#if !defined(UTH_PAGE_ALIGNED_STACK)
	/* Make stack 64-byte aligned */
	if ((unsigned long)sp & 0x3f)
		sp = sp - ((unsigned long)sp & 0x3f));     
	stack->sp = sp - _UTH_STACK_PAD_SIZE;
#else
	/* Make stack page aligned */
	if ((unsigned long)sp & (_UTH_PAGE_SIZE-1))  
		sp = sp - ((unsigned long)sp & (_UTH_PAGE_SIZE-1));
	stack->sp = sp; /*- _UTH_STACK_PAD_SIZE;*/
#endif
#elif defined(MD_STACK_GROWS_UP)
	sp = stack->stk_bottom;
	thread = (uth_t *) sp;
	sp = sp + sizeof(uth_t);

#if !defined(UTH_PAGE_ALIGNED_STACK)
	/* Make stack 64-byte aligned */
	if ((unsigned long)sp & 0x3f)
		sp = sp + (0x40 - ((unsigned long)sp & 0x3f));
	stack->sp = sp + _UTH_STACK_PAD_SIZE;
#else
	/* Make stack page-byte aligned (sparc -> 8192!!)*/
	if ((unsigned long)sp & (_UTH_PAGE_SIZE-1))
		sp = sp + ((_UTH_PAGE_SIZE) - ((unsigned long)sp & (_UTH_PAGE_SIZE-1)));
	stack->sp = sp; /*+ _UTH_STACK_PAD_SIZE;*/
#endif
#else
#error Unknown OS
#endif
	memset(thread, 0, sizeof(uth_t));

	/* Initialize thread */
	memcpy(&thread->stackinfo, stack, sizeof(uth_stack_t));
	thread->stack = &thread->stackinfo;
	thread->start = start;
	thread->arg = arg;

	thread->retval = (void *)0xDEAD;







#ifndef __ia64__
	_UTH_INIT_CONTEXT(thread, stack->sp, _uth_main);
#else
	_UTH_INIT_CONTEXT(thread, stack->sp, stack->bsp, _uth_main);
#endif
	return thread;
}

/* Thread reinitialization: set routine and argument, initialize context */
void uth_reinit(uth_t *thread, void (*start)(void *arg), void *arg)
{
	thread->start = start;
	thread->arg = arg;
#ifndef __ia64__
	_UTH_INIT_CONTEXT(thread, thread->stack->sp, _uth_main);
#else
	_UTH_INIT_CONTEXT(thread, thread->stack->sp, thread->stack->bsp, _uth_main);
#endif
}

void uth_restart(uth_t *thread)	
{
#ifndef __ia64__
	_UTH_INIT_CONTEXT(thread, thread->stack->sp, _uth_main);
#else
	_UTH_INIT_CONTEXT(thread, thread->stack->sp, thread->stack->bsp, _uth_main);
#endif
}

/* Delete (or recycle) thread */
void uth_delete(uth_t *thread)
{
#if defined(UTH_REUSE)
	_uth_put_reused_thread(thread);
#else
	_uth_stack_free(thread->stack);
#endif
}

/* 
 * Context switch: from old (i.e. current) to next. 
 * If old == NULL, switch to next without saving the
 * execution state of the old thread
 */
void uth_switchto(uth_t *old, uth_t *next)
{
	int vp;

	if (old) vp = old->vp_id;
	else	 vp = uth_get_vpid_tsd();
	/*if (vp < 0 || vp > MAX_VPS);*/

#if defined(UTH_CTXSW_SJLJ)
#if defined(WIN32_MSVC)	/* This is only for windows */
	unsigned long regs[6];
#endif
#endif

	if (next == NULL) {
		printf("uth_switchto: next == NULL\n");
		exit(1);
	}

#if defined(UTH_CTXSW_SJLJ)
#if defined(WIN32_MSVC)	/* This is only for windows */
	memcpy(regs, &next->context, sizeof(regs));
	MD_SETJMP(next->context);	/* re-init */
	memcpy(&next->context, regs, sizeof(regs));
#endif
#endif

	next->vp_id = vp;
#if defined(UTH_SI_TSD)
	pthread_setspecific(_uth_curr_key, next);
#endif





	if (old != NULL) {
		_UTH_SWITCH_CONTEXT(vp, old, next);	/* save and switch */
	}
	else {
		_UTH_RESTORE_CONTEXT(vp, next);		/* just switch */
	}
}

/* Self identification */
void uth_self_identification_method(void)
{
#if defined(UTH_SI_SB)
	printf("UTH_SI_SB\n");
#elif defined(UTH_SI_TSD)
	printf("UTH_SI_TSD\n");
#elif defined(UTH_SI_VPID)
	printf("UTH_SI_VPID\n");
#endif

#if defined(UTH_SI_KEEP_CURRENT_THREAD)
	printf("UTH_SI_KEEP_CURRENT_THREAD\n");
#endif
}


int _uth_get_vpid_tsd(void)
{
	int vp;
	vp = (int) (intptr_t)pthread_getspecific(_uth_vp_key);

	return vp;
}

int uth_get_vpid_tsd(void) { return _uth_get_vpid_tsd(); }

/* Get the identifier (vpid) of the current Posix Thread */
int uth_get_vpid(void)
{
	uth_t *thread;
	int vp;

	_uth_get_thread_vp(thread, vp);

	return vp;

	/* Not reached */
	return -1;
}

uth_t *_uth_self_from_stack()           
{     
	unsigned long sp;
#ifdef MD_STACK_GROWS_DOWN   
	unsigned long top;             
#else
	unsigned long bottom;
#endif
	uth_t *thread;

//	pthread_mutex_lock(&mm);
//	pthread_spin_lock(&ll);
	sp = (unsigned long) &sp;

//	printf("[%p] A sp 0x%lx main_sp 0x%lx  (%ld)\n", pthread_self(), sp, main_sp, abs(sp - main_sp));

	unsigned long distance;

	distance = (sp >= main_sp) ? sp - main_sp : main_sp - sp;

	if (distance < 1024UL * 1024UL) {
//	if (abs(sp - main_sp) < 1024*1024) {
//		printf("[%p] B sp 0x%lx main_sp 0x%lx  (%ld)\n", pthread_self(), sp, main_sp, abs(sp - main_sp));
		thread = main_uth;
////		printf("%s: returning main_uth:%p\n", __FUNCTION__, main_uth);
		//printf("[%p] : thread thread->retval: %p %p\n", pthread_self(), thread, thread->retval);
		return thread;
	}

//	pthread_mutex_unlock(&mm);
//	pthread_spin_unlock(&ll);

#ifdef MD_STACK_GROWS_DOWN
//	top = ((sp + _UTH_STACK_SIZE - 1) / _UTH_STACK_SIZE) * _UTH_STACK_SIZE; // top
	top = ((sp + _UTH_STACK_SIZE -1) >> ssizebits) << ssizebits;

	thread = (uth_t *) (top - sizeof(uth_t));
#else
	bottom = sp - (sp & (_UTH_STACK_SIZE-1));
	thread = (uth_t *) bottom;
#endif

	//printf("[%p] : thread thread->retval: %p %p\n", pthread_self(), thread, thread->retval);

	return thread;
}

uth_t *uth_self()
{
	uth_t *thread;
	int vp;

	_uth_get_thread_vp(thread, vp);
	return thread; 
}

/* Return thread argument: can be also used for thread local storage */
void *uth_getarg(uth_t *thread)
{
	return thread->arg;
}

/* Set thread argument */
void uth_setarg(uth_t *thread, void *arg)
{
	thread->arg = arg;
}


/* 
 * Secondary (extended) Interface:
 * Avoid calling uth_get_vpid() when the vpid of the current Posix thread
 * (virtual processor) is already known. For instance, this could happen
 * if the runtime system built on top of uthlib holds this information
 */

uth_t *uth_self_ex(int vp)
{
#if defined(UTH_SI_KEEP_CURRENT_THREAD)
	return _UTH_CURRENT_THREAD(vp);
#else
	return uth_self();
#endif
}

void *uth_getarg_ex(int vp)
{
	uth_t *thread;

#if defined(UTH_SI_KEEP_CURRENT_THREAD)
	thread =  _UTH_CURRENT_THREAD(vp);
#else
	thread = uth_self();
#endif
	return thread->arg;
}


void uth_switchto_ex(int vp, uth_t *prev, uth_t *next)
{
#if defined(UTH_CTXSW_SJLJ)
#if defined(WIN32_MSVC)	/* This is only for windows */
	unsigned long regs[6];
#endif
#endif
	if (next == NULL) {
		printf("uth_switchto: next == NULL\n");
		exit(1);
	}

#if defined(UTH_CTXSW_SJLJ)
#if defined(WIN32_MSVC)	/* This is only for windows */
	memcpy(regs, &next->context, sizeof(regs));
	MD_SETJMP(next->context);	/* re-init */
	memcpy(&next->context, regs, sizeof(regs));
#endif
#endif

	next->vp_id = vp;
#if defined(UTH_SI_TSD)
	pthread_setspecific(_uth_curr_key, next);
#endif





	if (prev != NULL) {
		_UTH_SWITCH_CONTEXT(vp, prev, next);	/* save and switch */
	}
	else {
		/* 'prev' is NULL but actually available and can be passed through.
		 * The previous thread must be recycled: next->old_uth = old, and
		 * when 'next' restarts can recycle it.
		 */
		_UTH_RESTORE_CONTEXT(vp, next);		/* just switch */
	}
}


/* 
 * Return elapsed time in sec
 */

#if defined(WIN32_MSVC) || defined(MINGW)
LARGE_INTEGER TicksPerSecond = {0,0}; /* Global ticks per second */
double TicksPerMicrosecond = 0;       /* Global ticks/microsecond */

double ElapsedMicroseconds()
{
	LARGE_INTEGER StopTime;
	double delta;
	
	QueryPerformanceCounter(&StopTime); /* read current clock */
	if (TicksPerSecond.LowPart == 0) {
		QueryPerformanceFrequency(&TicksPerSecond);
		TicksPerMicrosecond = (float) (TicksPerSecond.LowPart/ 1E6);
	}
	delta = StopTime.QuadPart / TicksPerMicrosecond; /* low part */
	
	return delta;
}
#endif

double uth_gettime(void)
{
#if defined(WIN32_MSVC) || defined(MINGW)
	return ElapsedMicroseconds() / 1E6;
#else
	struct timeval t;
	gettimeofday(&t, NULL);
	return (double)t.tv_sec + (double)t.tv_usec*1.0E-6;
#endif
}


/* 
 * Sleep for ms milliseconds (kernel level)
 */
void uth_vp_sleep(int ms)
{
#if defined(WIN32_MSVC) || defined(MINGW)
	Sleep(ms);
#else
	struct timespec ts;

	ts.tv_sec = ms/1000;
	ts.tv_nsec = (ms%1000)*1E6;
	nanosleep(&ts,NULL);
#endif
}

unsigned long uth_vp_self(void)
{
#if !defined(WIN32_MSVC) && !defined(MINGW)
	return (unsigned long) pthread_self();
#else
	return (unsigned long) pthread_getw32threadhandle_np(pthread_self());
#endif
}


void uth_info(void)
{
	char os[64];
	char ctxsw[64];
	
	strcpy(os,
#if defined (AIX)
	"AIX"
#elif defined (CYGWIN)
	"CYGWIN"
#elif defined (MINGW)
	"MINGW"
#elif defined (DARWIN)
	"DARWIN"
#elif defined (FREEBSD)
	"FREEBSD"
#elif defined (HPUX)
	"HPUX"
#elif defined (IRIX)
	"IRIX"
#elif defined (LINUX)
	"LINUX"
#elif defined (NETBSD)
	"NETBSD"
#elif defined (OPENBSD)
	"OPENBSD"
#elif defined (OSF1)
	"OSF1"
#elif defined (SOLARIS)
	"SOLARIS"
#elif defined (WIN32_MSVC) /* From st-win32 */
	"WIN32_MSVC"
#elif defined (QNX)	/* Panagiotis E. Hadjidoukas */
	"QNX"
#endif /* OS */
	);

#if defined(UTH_CTXSW_SJLJ)
#if defined(MD_USE_BUILTIN_SETJMP)
	strcpy(ctxsw, "builtin sjlj");
#else
	strcpy(ctxsw, "libc sjlj");
#endif
#elif defined(UTH_CTXSW_MCSC)
	strcpy(ctxsw, "mcsc");
#endif

	printf(">>>> Uth info: Operating System: %s, Context-switch method: %s <<<<\n\n", os, ctxsw);
}

void uth_stats()
{
#if defined(UTH_LOCKED_REUSE)
	printf("UTH_LOCKED_REUSE\n");
#endif
#if defined(UTH_STATS)
	int i;
	printf("================\n");
	printf("UTH_DESCRIPTORS:\n");
	for (i = 0; i <= _uth_max_vp; i++) {
		printf("vp %2d: created=%-5d reused=%-5d queued=%-5d\n", i, stacks_created(i), stacks_reused(i), stacks_queued(i));
	}
#endif
}
