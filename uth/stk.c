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
 * modified extensively by Panagiotis E. Hadjidoukas for PSthreads/UthLib. 
 */

#include "uth_common.h"
#include "uth_opt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <malloc.h>
#if !defined(WIN32_MSVC) && !defined(MINGW)
#include <sys/mman.h>
#endif
/* How much space to leave between the stacks, at each end */
#define REDZONE 0
/*_UTH_PAGE_SIZE*/

static char *_uth_new_stk_segment(int size, void *raddr);

/*
 * Create a new stack
 */
uth_stack_t *_uth_stack_new(uth_stack_t *stackinfo, int stack_size)
{
	uth_stack_t *ts = stackinfo;
	
	ts->vaddr_size = stack_size + 2*REDZONE;
	ts->vaddr = _uth_new_stk_segment(ts->vaddr_size /*+ _ST_PAGE_SIZE*/, &ts->raddr);
	if (!ts->vaddr) {
		printf("_uth_new_stk_segment returned NULL!\n");
		return NULL;
	}
	ts->stk_size = stack_size;
	ts->stk_bottom = ts->vaddr + REDZONE;
#if defined(ALLOCATE_DOUBLE_STACK_SIZE)
        ts->stk_bottom = (char *) (((((unsigned long)ts->stk_bottom) + (long)stack_size - 1) / (long)stack_size) * (long)stack_size);
#endif
	ts->stk_top = ts->stk_bottom + stack_size;
	
/*
#ifdef DEBUG
	mprotect(ts->vaddr, REDZONE, PROT_NONE);
	mprotect(ts->stk_top, REDZONE, PROT_NONE);
#endif
*/
	return ts;
}



uth_stack_t *_uth_stack_new_sp(uth_stack_t *stackinfo, int stack_size, char *vaddr)
{
	uth_stack_t *ts = stackinfo;
	
	ts->vaddr_size = stack_size + 2*REDZONE;
	ts->vaddr = ts->raddr = vaddr;
	if (!ts->vaddr) {
		printf("_uth_new_stk_segment returned NULL!\n");
		return NULL;
	}
	ts->stk_size = stack_size;
	ts->stk_bottom = ts->vaddr + REDZONE;
#if defined(ALLOCATE_DOUBLE_STACK_SIZE)
        ts->stk_bottom = (char *) (((((unsigned long)ts->stk_bottom) + (long)stack_size - 1) / (long)stack_size) * (long)stack_size);
#endif
	ts->stk_top = ts->stk_bottom + stack_size;
	
/*
#ifdef DEBUG
	mprotect(ts->vaddr, REDZONE, PROT_NONE);
	mprotect(ts->stk_top, REDZONE, PROT_NONE);
#endif
*/
	return ts;
}

/*
 * Free the stack of a finished thread
 */
void _uth_stack_free(uth_stack_t *ts)
{
	if (!ts)
		return;

#if defined(UTH_MALLOC_STACK)
	free(ts->raddr);
#else
	(void) munmap(ts->raddr, ts->vaddr_size);
#endif
}

/*
 * Allocate memory for a new stack
 */

static char *_uth_new_stk_segment(int size, void *raddr)
{
#if defined(UTH_MALLOC_STACK)
	void *vaddr;

#if defined(UTH_SI_SB)
	vaddr = _ort_calloc_aligned_ex(size, size, raddr);
//	vaddr = raddr = memalign(size, size);
#else
	vaddr = raddr = malloc(size);	/* calloc ? */
#endif
#else   /* MMAP - not used */
	static int zero_fd = -1;
	int mmap_flags = MAP_PRIVATE;
	void *vaddr;

#if defined (MD_USE_SYSV_ANON_MMAP)
        if (zero_fd < 0) {
                if ((zero_fd = open("/dev/zero", O_RDWR, 0)) < 0)
                        return NULL;
                fcntl(zero_fd, F_SETFD, FD_CLOEXEC);
        }
#elif defined (MD_USE_BSD_ANON_MMAP)
        mmap_flags |= MAP_ANON;
#else
#error Unknown OS
#endif

#if defined(UTH_SI_SB)
	raddr = mmap(NULL, 2*size, PROT_READ | PROT_WRITE, mmap_flags, zero_fd, 0);
#else
	raddr = mmap(NULL, size, PROT_READ | PROT_WRITE, mmap_flags, zero_fd, 0);
#endif
	if (raddr == MAP_FAILED) {
		raddr = NULL;
		return NULL;	/* vaddr */
	}
#if defined(UTH_SI_SB)
	vaddr = (void *) ((((unsigned long)raddr + size - 1) / size) * size);
#else
	vaddr = raddr;
#endif

#endif /* UTH_MALLOC_STACK */

/**	memset(vaddr, 0, size);**/	/* peh: ??? */

	return (char *)vaddr;
}
