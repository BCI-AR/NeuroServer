# nsd is the NeuroServer daemon

AM_CFLAGS= -Wall -g
AM_CPPFLAGS= -I.

lib_LIBRARIES = libneuro.a

libneuro_a_HEADERS = neuro/neuro.h neuro/stringtable.h neuro/cmdhandler.h neuro/ns2net.h
libneuro_adir = $(includedir)/neuro

libneuro_a_SOURCES = stringtable.c cmdhandler.c ns2net.c reader.c nsrealloc.c

bin_PROGRAMS = nsd modeegdriver readedf writeedf opencnvdriver writebdf ntest

ntest_SOURCES = ntest.c
ntest_LDADD = libneuro.a

nsd_SOURCES = nsd.c nsutil.c nsnet.c monitor.c monitor.h \
openedf.c config.h edfmacros.h nsd.h nsnet.h nsutil.h openedf.h pctimer.h
nsd_LDADD = libneuro.a

modeegdriver_SOURCES = modeegdriver.c nsser.c nsutil.c openedf.c \
nsnet.c monitor.c monitor.h \
config.h edfmacros.h nsnet.h nsser.h nsutil.h openedf.h

opencnvdriver_SOURCES = opencnvdriver.c nsser.c nsutil.c openedf.c \
nsnet.c monitor.c monitor.h \
config.h edfmacros.h nsnet.h nsser.h nsutil.h openedf.h

readedf_SOURCES = readedf.c nsutil.c nsnet.c monitor.c monitor.h openedf.c \
pctimer.c config.h edfmacros.h nsnet.h nsutil.h pctimer.h openedf.h

writeedf_SOURCES = writeedf.c nsutil.c nsnet.c monitor.c monitor.h \
pctimer.c openedf.c config.h edfmacros.h openedf.h nsutil.h nsnet.h pctimer.h

writebdf_SOURCES = writebdf.c nsutil.c nsnet.c monitor.c monitor.h \
pctimer.c openedf.c config.h edfmacros.h openedf.h nsutil.h nsnet.h pctimer.h
