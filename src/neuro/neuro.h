#ifndef __NEURO_H
#define __NEURO_H

#define OK 0

#define rassert(x) if (!(x)) { void exit(int); fprintf(stderr, "FAIL: %s:%d:%s\n", __FILE__, __LINE__, #x); exit(1); }

#include <neuro/stringtable.h>
#include <neuro/cmdhandler.h>
#include <neuro/ns2net.h>

#endif
