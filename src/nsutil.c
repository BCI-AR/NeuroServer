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
                
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include "nsutil.h"

#ifdef __MINGW32__
#include <windows.h>
#else
#include <unistd.h>
#endif

int max_fd = 0;

void rtime(time_t *t)
{
	time(t);
}

void updateMaxFd(int fd)
{
	if (max_fd < fd + 1)
		max_fd = fd + 1;
}

int rprintf(const char *fmt, ...)
{
	int retval;
	char buf[4096];
	va_list va;
	buf[4095] = '\0';
	va_start(va, fmt);
	vsnprintf(buf, 4095, fmt, va);
	va_end(va);
	retval = fprintf(stdout, "%s", buf);
	fflush(stdout);
	return retval;
}

int rexit(int errcode) {
	printf("Exitting with error code %d\n", errcode);
	fflush(stdout);
	*((char *) 0) = 0;
	assert(0);
	return 0;
}
void rsleep(int ms)
{
#ifdef __MINGW32__
	Sleep(ms);
#else
	usleep(ms*10000);
#endif
}
