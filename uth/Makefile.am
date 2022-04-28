#
#    PSthreads and UthLib thread libraries
#    == Copyright (C) 2007 the OMPi/PSthreads Team
#    == Department of Computer Science, University of Ioannina
#
#    This file is part of PSthreads.
#
#    PSthreads is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    PSthreads is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with PSthreads; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
##   This file must be processed by automake

uthlibdir = $(libdir)
uthlib_LIBRARIES=libuth.a

AM_CFLAGS = @DEBUG_FLAG@ -D@OS@ -DMAX_VPS=@NVPS@ @USEASMSJLJ@ -I../include -I. 

if PTH
libuth_a_SOURCES = sched_pth.c 
else

if WIN32_MCSC
libuth_a_SOURCES = sched.c stk.c win32_ucontext.c
else

if COMPILE_MD_S
AM_CCASFLAGS = -D@OS@
libuth_a_SOURCES = sched.c stk.c md.S
else
libuth_a_SOURCES = sched.c stk.c
endif

endif

endif

uthincdir = $(includedir)
uthinc_HEADERS = uth.h

EXTRA_DIST = md.h uth_common.h uth_opt.h
