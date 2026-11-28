/*
  OMPi OpenMP Compiler
  == Copyright since 2001 the OMPi Team
  == Department of Computer Science, University of Ioannina

  This file is part of OMPi.

  OMPi is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  OMPi is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OMPi; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef __SYSDEPS_H__
#define __SYSDEPS_H__

#include "../include/config.h"
//#include <unistd.h>


/*
 * Processors
 */
#if defined(pentium4) || defined(__pentium4)
  #define __cpu_x86 686
#elif defined(i686) || defined(__i686) || \
    defined(pentiumpro) || defined(__pentiumpro) || \
    defined(pentium2) || defined(__pentium2) || \
    defined(pentium3) || defined(__pentium3)
  #define __cpu_x86 686
  #define __cpu_686_class p6       /* Only used for CACHE_LINE size */
#elif defined(i586) || defined(__i586) || defined(pentium) || defined(__pentium)
  #define __cpu_x86 586
#elif defined(i486) || defined(__i486)
  #define __cpu_x86 486
#elif defined(i386) || defined(__i386)
  #define __cpu_x86 386
#elif defined(__x86_64)
  #define __cpu_x86 686
#endif

#if defined(sparcv9) || defined(__sparcv9)
  #define __cpu_sparc v9
#elif defined(sparc) || defined(__sparc)
  #define __cpu_sparc v8
#endif

#if defined(mips) || defined(__mips)
  #define __cpu_mips 1
#endif

/* 
 * A memory fence implementation;
 * We basically assume a gcc-compatible compiler for inline assembly.
 */
#if defined(__cpu_x86)
  #if defined(__SYSCOMPILER_sun)
    #define FENCE __asm("cpuid")
  #else /* Assume gcc compatible */
    #define FENCE asm volatile("cpuid" : : : "%eax", "%ebx", "%ecx", "%edx")
  #endif
#elif defined(__cpu_sparc)
  #if defined(__SYSCOMPILER_sun)
    #if __cpu_sparc == v8  
      #define FENCE __asm("stbar")
    #else
      #define FENCE __asm("membar #Sync")
    #endif
  #else
    #if __cpu_sparc == v8  
      #define FENCE asm volatile("stbar")
    #else
      #define FENCE asm volatile("membar #Sync")
    #endif
  #endif
#elif defined(__aarch64__) || defined(__arm64__)
  #define FENCE __asm__ __volatile__("dmb ish" ::: "memory")
#elif defined(__cpu_mips)
  #ifdef __sys_sgi
    /* Most (?) SGI systems, such as the Origins & Onyxes guarantee
     * sequential consistency so nothing needs to be done. 
     */
    #define FENCE  
  #else
    #define FENCE asm volatile("sync")
  #endif
#else
  /* We have to play safe; a POSIX-threads way of guaranteeing sequential
   * consistency is by locking & unlocking a mutex. Too much overhead
   * but what else can we do?
   */
#error "mutex"
  #define __FENCE_MUTEX__          /* Used in sysdeps.c */
  extern void _bad_fence(void);
  #define FENCE _bad_fence()
#endif

#endif  /* __SYSDEPS_H__ */

