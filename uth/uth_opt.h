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

/*! \file uth_opt.h
    \brief UthLib options
 */


#ifndef __UTH_OPT_H__
#define __UTH_OPT_H__

/* Maximum kernel threads (virtual processors) supported */
#ifndef MAX_VPS
#define MAX_VPS	16
#endif

/* Stack */
#ifndef UTH_DEFAULT_STACK_SIZE
//#define UTH_DEFAULT_STACK_SIZE (4*1024*1024)		/* 4MB */
#define UTH_DEFAULT_STACK_SIZE (128*1024)		/* 64KB */
#endif

//#define UTH_STATS		/* defined in config.h */

#define UTH_LOCKED_REUSE

#endif
