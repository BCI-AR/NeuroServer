/*
NeuroServer
 
A collection of programs to translate between EEG data and TCP network
messages. This is a part of the OpenEEG project, see http://openeeg.sf.net
for details.
    
Copyright (C) 2003, 2004 Rudi Cilibrasi (cilibrar@ofb.net)
     
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.
         
This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.
             
You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*/
                
#ifndef __NSUTIL_H
#define __NSUTIL_H

#include <time.h>
#include "pctimer.h"

#define MAXLEN 16384

int rprintf(const char *fmt, ...);
int rexit(int errcode);
extern int max_fd;
void updateMaxFd(int fd);
void rtime(time_t *t);
void rsleep(int ms);

#if defined(__MINGW32__) || defined(__CYGWIN__)
#define OSTYPESTR "Windows"
#else
#define OSTYPESTR "Linux"
#endif

#endif
