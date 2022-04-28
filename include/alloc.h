/*
  OMPi OpenMP Compiler
  == Copyright (C) 2001, 2003, 2004, 2007 the OMPi Team
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

/*! \file allow.h
    \brief Memory allocation
 */

#ifndef __ALLOC_H__
#define __ALLOC_H__

#include "ps_config.h"
#include <stdio.h>
#include <stdlib.h>

/* borrowed from OMPi's lib/ort.c */
static void * _ort_calloc(int size)
{ 
  void *a;

  if ((a = calloc(1, size)) == NULL) {
    printf("[PSTHREADS]: memory allocation failed\n"); exit(1);
  }
  return (a);
} 

static void * _ort_calloc_aligned(int size, void **actual)
{
	if (actual == NULL) {
		void *tmp;

		tmp = _ort_calloc(size + CACHE_LINE_SIZE - 1);
		return ( (void *) ( (((unsigned long) (tmp)) + CACHE_LINE_SIZE - 1) & (-CACHE_LINE_SIZE) ) );
	}
	*actual = _ort_calloc(size + CACHE_LINE_SIZE - 1);
	return ( (void *) ( (((unsigned long) (*actual)) + CACHE_LINE_SIZE - 1) & (-CACHE_LINE_SIZE) ) );
}

static void * _ort_calloc_aligned_ex(int size, int alignment, void **actual)
{
	if (actual == NULL) {
		void *tmp;

		tmp = _ort_calloc(size + alignment - 1);
		return ( (void *) ( (((unsigned long) (tmp)) + alignment - 1) & (-alignment) ) );
	}
	*actual = _ort_calloc(size + alignment - 1);
	return ( (void *) ( (((unsigned long) (*actual)) + alignment - 1) & (-alignment) ) );
}

#endif
