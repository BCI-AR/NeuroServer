/*!  \brief serial port communication for eeg devices 
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
#include <assert.h>
#include <stdlib.h>
#include "nsutil.h"
#include "nsser.h"



ser_t openSerialPort(const char *devname, unsigned int BaudRate)
{
#ifdef __MINGW32__
	ser_t retval;
	retval = CreateFile(devname,
        GENERIC_READ | GENERIC_WRITE,
	0, NULL, OPEN_EXISTING,
	    FILE_ATTRIBUTE_NORMAL, NULL);
	if (retval == INVALID_HANDLE_VALUE) {
    	 rprintf("Couldn't open serial port: %s", devname);
		rexit(1);
	}
	if (!SetCommMask(retval, EV_RXCHAR))
	rprintf("Couldn't do a SetCommMask(): %s", devname);

	// Set read buffer == 10K, write buffer == 4K
	SetupComm(retval, 10240, 4096);

	do {
	DCB dcb;

	dcb.DCBlength= sizeof(DCB);
	GetCommState(retval, &dcb);
	switch(BaudRate) {
	 case 19200: dcb.BaudRate=CBR_19200;break;
	 case 57600: dcb.BaudRate=CBR_57600;break;
	 case 115200: dcb.BaudRate=CBR_115200;break;
	 default: dcb.BaudRate=CBR_57600;
	} 
	        dcb.ByteSize= 8;
		dcb.Parity= NOPARITY;
		dcb.StopBits= ONESTOPBIT;
		dcb.fOutxCtsFlow= FALSE;
		dcb.fOutxDsrFlow= FALSE;
		dcb.fInX= FALSE;
		dcb.fOutX= FALSE;
		dcb.fDtrControl= DTR_CONTROL_DISABLE;
		dcb.fRtsControl= RTS_CONTROL_DISABLE;
		dcb.fBinary= TRUE;
		dcb.fParity= FALSE;

	if (!SetCommState(retval, &dcb)) {
			rprintf("%s", "Unable to set up serial port parameters");
			rexit(1);
		}
	} while (0);
	return retval;
#else
	ser_t fd; /* File descriptor for the port */


	fd = open(devname, O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd == -1)
		{
		fprintf(stderr, "Cannot open port %s\n", devname);
		exit(1);
		}
	else
		fcntl(fd, F_SETFL, FNDELAY);
	set_port_options(fd,BaudRate);
	return (fd);
#endif
}
/* Returns number of bytes read, or -1 for error */
#ifdef __MINGW32__
int readSerial(ser_t s, char *buf, int size)
{
   BOOL rv;
   COMSTAT comStat;
   DWORD errorFlags;
   DWORD len;
   // Only try to read number of bytes in queue
   ClearCommError(s, &errorFlags, &comStat);
   len = comStat.cbInQue;
   if (len > size) len = size;
   if (len > 0) {
      rv = ReadFile(s, (LPSTR)buf, len, &len, NULL);
			if (!rv) {
				 len= 0;
				 ClearCommError(s, &errorFlags, &comStat);
			}
		if (errorFlags > 0) {
			rprintf("Serial read error");
			ClearCommError(s, &errorFlags, &comStat);
			return -1;
		}
	}
	return len;
}
#else

int readSerial(ser_t s, char *buf, int size)
{
	int retval;
	retval = read(s, buf, size);
	if (retval < 0)
		retval = 0;
	return retval;
}

/// \todo add write serial for windows and test it

int writeSerial(ser_t s, char *buf, int size)
{
	int retval;
	retval = write(s, buf, size);
	if (retval < 0)
		retval = 0;
	return retval;
}

int set_port_options(int fd, unsigned int BaudRate)
{
	int retval;
	struct termios options;
/*
 *      * Get the current options for the port...
 *      */

    retval = tcgetattr(fd, &options);
		assert(retval == 0 && "tcgetattr problem");

/*
 *      * Set the baud rates to 19200...
 *      */
	switch(BaudRate) {
	 case 19200:  cfsetispeed(&options, B19200);cfsetospeed(&options, B19200);break;
	 case 57600:  cfsetispeed(&options, B57600);cfsetospeed(&options, B57600);break;
	 case 115200:  cfsetispeed(&options, B115200);cfsetospeed(&options, B115200);break;
	 default: cfsetispeed(&options, B57600);cfsetospeed(&options, B57600);
	} 
   


/*
 *      * Enable the receiver and set local mode...
 *      */

    options.c_cflag |= (CLOCAL | CREAD);

/* 8-N-1 */
options.c_cflag &= ~PARENB;
options.c_cflag &= ~CSTOPB;
options.c_cflag &= ~CSIZE;
options.c_cflag |= CS8;

/* Disable flow-control */
  options.c_cflag &= ~CRTSCTS;

/* Raw processing mode */
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);





/*
 *      * Set the new options for the port...
 *      */

	retval = tcsetattr(fd, TCSANOW, &options);
	return retval;
}
#endif


