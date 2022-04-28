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

check_PROGRAMS=ps_block ps_condvar ps_test ps_bench ps_barr ps_self ps_wf ps_bunch

AM_CFLAGS = @DEBUG_FLAG@ -D@OS@ -I../include -I../psthread -I../uth -I.
LIBS = -L../psthread -lpsthread -L../uth -luth -lpthread

ps_block_SOURCES = ps_block.c
ps_block_LDADD = $(LIBS)

ps_condvar_SOURCES = ps_condvar.c
ps_condvar_LDADD = $(LIBS)

ps_test_SOURCES = ps_test.c
ps_test_LDADD = $(LIBS)

ps_bench_SOURCES = ps_bench.c
ps_bench_LDADD = $(LIBS)

ps_barr_SOURCES = ps_barr.c
ps_barr_LDADD = $(LIBS)

ps_self_SOURCES = ps_self.c
ps_self_LDADD = $(LIBS)

ps_wf_SOURCES = ps_wf.c
ps_wf_LDADD = $(LIBS)

ps_bunch_SOURCES = ps_bunch.c
ps_bunch_LDADD = $(LIBS)

