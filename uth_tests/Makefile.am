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

check_PROGRAMS=uth_creation uth_ctxsw uth_test uth_create_sp

AM_CFLAGS = @DEBUG_FLAG@ -D@OS@ -I../include -I../uth -I.
LIBS = -L../uth -luth -lpthread

uth_creation_SOURCES = uth_creation.c
uth_creation_LDADD =$(LIBS)

uth_ctxsw_SOURCES = uth_ctxsw.c
uth_ctxsw_LDADD = $(LIBS)

uth_test_SOURCES = uth_test.c
uth_test_LDADD = $(LIBS)

uth_create_sp_SOURCES = uth_create_sp.c
uth_create_sp_LDADD = $(LIBS)

