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

/*! \file uth.h
    \brief UthLib application programming interface
 */
 
#ifndef __UTH_H__
#define __UTH_H__

typedef void *uth_t;

#ifdef __cplusplus
extern "C" {
#endif

/* Primary API */
extern int uth_init(int stacksize);
extern uth_t uth_vp_init(int vp, void *arg);
extern int uth_get_vpid(void);
extern uth_t uth_create(void (*start)(void *arg), void *arg);
extern uth_t uth_create_sp(void (*start)(void *arg), void *arg, int stacksize, char *vaddr);
void uth_reinit(uth_t thread, void (*start)(void *arg), void *arg);
extern void uth_delete(uth_t t);
extern void uth_switchto(uth_t t1, uth_t t2);
extern uth_t uth_self(void);
extern uth_t _uth_self_from_stack(void);
extern void *uth_getarg(uth_t thread);
extern void uth_setarg(uth_t thread, void *arg);
extern double uth_gettime(void);

/* Extended API */
extern void uth_switchto_ex(int vp, uth_t t1, uth_t t2);
extern uth_t uth_self_ex(int vp);
extern void *uth_getarg_ex(int vp);

/* Helper routines */
extern void uth_vp_sleep(int ms);
unsigned long uth_vp_self(void);

extern void uth_stats(void);
extern void uth_info(void);
#ifdef __cplusplus
}
#endif

#endif /* !__UTH_H__ */


