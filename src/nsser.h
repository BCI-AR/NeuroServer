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
                
#ifndef __NSSER_H
#define __NSSER_H

#ifdef __MINGW32__
#define DEFAULTDEVICE "COM1"
#include <windows.h>
#include <winbase.h>
#define ser_t HANDLE
#else
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */
#define DEFAULTDEVICE "/dev/ttyS0"
#define ser_t int
#endif

/*! Pass the device name, if it returns it has succeeded */
ser_t openSerialPort(const char *devname, unsigned int BaudRate);

/*! Returns number of bytes read, or -1 for error */
int readSerial(ser_t s, char *buf, int size);

/*! Returns number of bytes written, or -1 for error */
int writeSerial(ser_t s, char *buf, int size);

/*! sets baudrate and other port options (unix only) */
int set_port_options(int fd, unsigned int BaudRate);

#endif
