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
#include <monitor.h>
#include <nsnet.h>

int monitorLog(enum PlaceCode placeCode, int errNum)
{
	char *descPlace;
	char buf[MAXLEN];
	sprintf(buf, "unknown error code: %d", errNum);
	switch (placeCode) {
		case PLACE_CONNECT: descPlace = "connect"; break;
		case PLACE_WRITEBYTES: descPlace = "writebytes"; break;
		case PLACE_MYREAD: descPlace = "myread"; break;
		case PLACE_SETBLOCKING: descPlace = "setblocking"; break;
		case PLACE_RRECV: descPlace = "rrecv"; break;
		case PLACE_RSELECT: descPlace = "rselect"; break;
		default:
			descPlace = buf;
			break;
	}
	fprintf(stdout, "Recieved error code %d from placeCode %d.  errstr is %s and descPlace is %s\n", errNum, placeCode, stringifyErrorCode(errNum), descPlace);
	fflush(stdout);
	return 0;
}

