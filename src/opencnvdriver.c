/** \file
 * \author Moritz von Buttlar (moritzvb@users.sf.net), based on the modeegdriver by Rudi Cilibrasi
 *
 * Device driver for the 24bit OpenCNV EEG.
 * 
 */

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
#include <assert.h>
#include <stdlib.h>
#include <string.h>
/// neuroserver network support
#include <nsnet.h>
#include <nsutil.h>
/// neuroserver serial port support
#include <nsser.h>
/// european data format support (e.g. header structures)
#include <openedf.h>
/// release number support
#include <config.h>
/// size of a serial packet 
#define PROTOWINDOW 12
/// Debugging switch, uncomment for debugging info 
//#define DEBUGGING
/// serial port input buffer
char buf[PROTOWINDOW];
/// handler for network socket
sock_t sock_fd;
int bufCount;
static int failCount = 0;
/// network output buffer
static struct OutputBuffer ob;
/// network input buffer
static struct InputBuffer ib;
/// protocol detection
int hasMatchedProtocol;
/// eeg device detection
int isOpenCNV;
/// if the -m switch is set, isPseudoEDF is set to 1 and 24bit data is splitted into 2 16bit channels
int isPseudoEDF;
/// checks data packet for validity (range)
int isValidPacket(unsigned short nchan, long *samples);
/// function for frame decoding
int doesMatchOpenCNV(unsigned char c, long *vals);
/// get neuroserver response
int mGetOK(sock_t fd, struct InputBuffer *ib);


/// get response from neuroserver
int mGetOK(sock_t fd, struct InputBuffer *ib)
{
	return 0;
}

/// standard configuration for EDF header
static struct EDFDecodedConfig openCNVCfg = {
		{ 0,   				// size of header in bytes (to be set later)
			-1,  			// data record count, -1 if unknown
			3,   			// channels
			"24bit", 		// data format version number
			"Mr. Default Patient",
			"Ms. Default Recorder",
			"date",
			"time",
			"24bit OpenCNV",	// manufactorer ID
			2,  			// seconds per data record
		}, {
				{
					1001, 		///< samples per record
					-512, 512, 	///< physical range
					-8388608, 8388607,///< digital range
					"electrode 0", 	///< label
					"Red Dot Ag/AgCl",  ///< transducer
					"uV",  		///< physical unit dimensions
					"LP:100 Hz", 	///< prefiltering
					""         	///< reserved
				},
				{
					1001, 		///< samples per record
					-512, 512, 	///< physical range
					-8388608, 8388607,   	///< digital range
					"electrode 1", 	///< label
					"Red Dot Ag/AgCl",  ///< transducer
					"uV",  		///< physical unit dimensions
					"LP:100 Hz", 	///< prefiltering
					""         	///< reserved
				},								
					{
					1001, 		///< samples per record
					0, 255, 	///< physical range
					0, 255,   	///< digital range
					"Stimulus State", 	///< label
					"----",  ///< transducer
					"State",  		///< physical unit dimensions
					"none", 	///< prefiltering
					""         	///< reserved
				}								
				
		}
	};

static struct EDFDecodedConfig current;

/*! \brief checks plausibility of input data
    \todo get maximum digital value from header
  */
int isValidPacket(unsigned short chan, long *samples)
{
	int i;
//	if (chan != 2 && chan != 4 && chan !=5 && chan != 6) return 0;
//	for (i = 0; i < chan; i++) {
//		if (samples[i] > 16777216) return 0;
//	}
	return 1;
}



/// this function does the real data processing of serial input data
void eatCharacter(unsigned char c)
{
/// number of frames until protocol is locked in
#define MINGOOD 6
	static int goodCount;
/// place to store eeg data 
	long vals[MAXCHANNELS];  
	int didMatch = 0;
	int ns=3;
/// has the protocol been detected and validated ?	
	if (hasMatchedProtocol) {
	   didMatch = doesMatchOpenCNV(c, vals);
	 } 
/// not yet, or error previously	 
	 else {
	   didMatch = doesMatchOpenCNV(c, vals); 
/// now it detected the frame, from now on the protocol is fixed
	   if (didMatch) {
	    if (goodCount > MINGOOD) {
	     hasMatchedProtocol = 1;
//	     isOpenCNV = 1;
            }
	   }
	 }
///	 
	if (didMatch) {	
		char *pstr = "xx";
/// check packet for validity		
//		if (isValidPacket(ns, vals)) {
//			goodCount += 1;
//		}
//		else {
//			rprintf("Warning: invalid serial packet ahead!\n");
//		}
		pstr = "OpenCNV";
//		failCount = 0;
//                printf("%c",c);	
#ifdef DEBUGGING1
 	        int i;	
		printf("Got %s packet with %d values: ", pstr, ns);
		for (i = 0; i < 3; ++i)
    		 printf("%ld ", vals[i]);
		printf("\n");
#endif		
	}
/// ok, didmatch is 0, so it didn't match so far	
	else {
		failCount += 1;
		if (failCount % PROTOWINDOW == 0) {
			rprintf("Serial packet sync error -- missed window.\n");
			goodCount = 0;
		}
	}
}


/** \brief sends sample values to the nsd daemon.

   chan Samples, stored in the long array *vals, will be send.
   The packetCounter information can be used by the application software to
   check for lost packets.	
  
  */
  
void handleSamples(int packetCounter, int chan, long *vals)
{
	char buf[MAXLEN];					///< network socket buffer
	int bufPos = 0;
        long ChannelValue;
	int i;
#ifdef DEBUGGING	
	printf("Got good packet with counter %d, %d chan\n", packetCounter, chan);
#endif	
        chan=3;
        bufPos+=sprintf(buf, "! %d %d", packetCounter, chan);
        for (i = 0; i < 3; i++) {
	  bufPos+=sprintf(buf+bufPos, " %ld",vals[i]); 
	 }
	bufPos+=sprintf(buf+bufPos, "\r\n");
	writeString(sock_fd, buf, &ob);
	mGetOK(sock_fd, &ib);
}

/// state machine for serial protocol decoding, get's called with every new character that has been
/// received
/// returns 1 if everything is fine, 0 if there has been some problem (e.g. data lost, frame not aligned right)

int doesMatchOpenCNV(unsigned char c, long *vals)
{
	enum P2State { waitingA5, waiting5A, waitingCounter,
		waitingData };
	static int packetCounter = 0;
	static int chan = 0;
	static int i = 0;
	static enum P2State state = waitingA5;

	switch (state) {
		case waitingA5:
			if (c == 0xa5) {
				failCount = 0;
				state = waiting5A;
			}
			else {
				failCount += 1;
				if (failCount % PROTOWINDOW == 0) {
					rprintf("Sync error, packet lost.\n");
					return 0;
				}
			}
			break;

		case waiting5A:
			if (c == 0x5a)
				state = waitingCounter;
			else
				state = waitingA5;
				return 0;
			break;


		case waitingCounter:
			packetCounter = c;
			i = 0;		
			state = waitingData;
	
			break;

		case waitingData:
		        switch(i) {
		         case 0: vals[0] = 256*256*c;i++;break;		         
		         case 1: vals[0]+=256*c;i++;break;
		         case 2: vals[0]+=c;i++;vals[0]=vals[0]-8388608;break;
		         case 3: i++;break;				///< don't check ADC Ch1 status
			 case 4: vals[1] = 256*256*c;i++;break;		         
		         case 5: vals[1]+=256*c;i++;break;
		         case 6: vals[1]+=c;vals[1]=vals[1]-8388608;i++;break;
		         case 7: i++;break;				///< don't check ADC Ch2 status
		         case 8: vals[2]=c;				///< get stimulus value
				 i++;
			   	 chan =  3; 
				 handleSamples(packetCounter, chan , vals);
			  	 state = waitingA5;break;
			  default:
			  	rprintf("Error: problem during waiting Data state\n");
				rexit(0);
		 	}
			break;
		default:
				rprintf("Error: unknown state in OpenCNV protocol!\n");
				rexit(0);
		}
	return 1;
}

/// prints command line options / brief description of the program
void printHelp()
{
	rprintf("OpenCNV Driver interfaces the OpenCNV system to the NeuroServer TCP/IP protocol\n");
	rprintf("Usage: opencnvdriver [-d serialDeviceName] [-m} [hostname] [portno]\n");
	rprintf("The default options are opencnvdriver -d %s %s %d\n",
			DEFAULTDEVICE, DEFAULTHOST, DEFAULTPORT);
	rprintf("The -m option splits 24bit ADC data into an offset part and regular part, \n");
	rprintf("this allows the standard 16bit EDF format to be used with higher dynamic range data.");
	
	
			
}

void decodeCommand(char command[MAXLEN],ser_t serport) {
char writebuf[2];

 if ((command[0]=='g') && (command[1]=='o')) {
  writebuf[0]='T';
  writebuf[1]='1';
  writeSerial(serport,writebuf,2);
  }
 
 if ((command[0]=='n') && (command[1]=='o') && (command[2]=='g') && (command[3]=='o')) {
  writebuf[0]='T';
  writebuf[1]='3';
  writeSerial(serport,writebuf,2);
  }
  
  
 if ((command[0]=='2') && (command[1]=='0') && (command[2]=='0') ) {
//  rprintf("OK\n");
 }
 else {
  rprintf("not OK: %c %c %c \n",command[0],command[1],command[2]);
 }
}


int main(int argc, char **argv)
{
	char responseBuf[MAXLEN];
	int i;
	int retval;
	ser_t serport;
	char EDFPacket[MAXHEADERLEN];
	int EDFLen = MAXHEADERLEN;
	char smallbuf[PROTOWINDOW];
	/// DEFAULTHOST
	char *hostname = NULL;
	char *deviceName = NULL;
	/// DEFAULTPORT
	unsigned short portno = 0;
	struct timeval  when;
	
        char linebuf[MAXLEN+1];	// fifo command buffer
        int writepos = 0;        
        int readpos = 0;
        int len;
        unsigned int lpos = 0;
	
	rprintf("OpenCNV Driver v. %s-%s\n", VERSION, OSTYPESTR);

	rinitNetworking();
	
        isPseudoEDF=0;
	for (i = 1; argv[i]; ++i) {
		if (argv[i][0] == '-' && argv[i][1] == 'h') {
			printHelp();
			exit(0);
		}
		if (argv[i][0] == '-' && argv[i][1] == 'd') {
			if (argv[i+1] == NULL) {
				rprintf("Bad devicename option: %s\nExpected device name as next argument.\n", argv[i]);
				exit(1);
			}
			deviceName = argv[i+1];
			i += 1;
			continue;
		}
		if (argv[i][0] == '-' && argv[i][1] == 'm') {
		        isPseudoEDF=1;
			continue;

		}
		if (hostname == NULL) {
			hostname = argv[i];
			continue;
		}
		if (portno == 0) {
			portno = atoi(argv[i]);
			continue;
		}
	}
	if (deviceName == NULL)
		deviceName = DEFAULTDEVICE;
	if (portno == 0)
		portno = DEFAULTPORT;
	if (hostname == NULL)
		hostname = DEFAULTHOST;
///	
	current=openCNVCfg;
	setEDFHeaderBytes(&current);
        strcpy(current.hdr.recordingStartDate, getDMY());
        strcpy(current.hdr.recordingStartTime, getHMS());
//        assert(isValidREDF(result));
//  	makeREDFConfig(&current, &openCNVCfg);
//	write configuration into string 
	writeEDFString(&current, EDFPacket, &EDFLen);


	sock_fd = rsocket();
	if (sock_fd < 0) {
		perror("socket");
		rexit(1);
	}
	setblocking(sock_fd);

	rprintf("Attempting to connect to nsd at %s:%d\n", hostname, portno);
	retval = rconnectName(sock_fd, hostname, portno);
	if (retval != 0) {
		rprintf("connect error\n");
		rexit(1);
	}
	rprintf("Socket connected.\n");
	fflush(stdout);

	serport = openSerialPort(deviceName,115200);			
	rprintf("Serial port %s opened.\n", deviceName);
	writeString(sock_fd, "eeg\n", &ob);	/// now start talking with the nsd
	mGetOK(sock_fd, &ib);
	writeString(sock_fd, "setheader ", &ob);
/// send header	
	writeBytes(sock_fd, EDFPacket, EDFLen, &ob);
	writeString(sock_fd, "\n", &ob);
	mGetOK(sock_fd, &ib);
	updateMaxFd(sock_fd);
#ifndef __MINGW32__
	updateMaxFd(serport);
#endif
	when.tv_sec = 0;
	when.tv_usec = (int)(1000000 / openCNVCfg.chan[0].sampleCount * openCNVCfg.hdr.dataRecordSeconds);
	rprintf("Polling at %f Hertz or %i usec\n",(1e6/when.tv_usec), 
	     when.tv_usec);
	for (;;) {
		int i, readSerBytes;
		int retval;
		fd_set toread;
		FD_ZERO(&toread);
		FD_SET(sock_fd, &toread);
#ifndef __MINGW32__
		FD_SET(serport, &toread);
#endif
		when.tv_usec = (int)(1000000 / openCNVCfg.chan[0].sampleCount * openCNVCfg.hdr.dataRecordSeconds);
		when.tv_sec = 0;
		retval = rselect_timed(max_fd, &toread, NULL, NULL, &when);
		readSerBytes = readSerial(serport, smallbuf, PROTOWINDOW);		// get one frame
// timeout + exit, if no serial data available
		if (readSerBytes < 0) {
			rprintf("Serial error.\n");
		}
		if (readSerBytes > 0) {
			for (i = 0; i < readSerBytes; ++i)
				eatCharacter(smallbuf[i]);    /// process serial port input data
		}
/// code to get commands from server 
		if ( (FD_ISSET(sock_fd, &toread)) && (ib.read_cnt<=0) ) { 	// did we get any new network data ? yes !
			my_read(sock_fd, responseBuf, MAXLEN, &ib); // get data into input buffer
			len=0;
			for (i=0;i<ib.read_cnt;i++) {
			 linebuf[writepos]=ib.read_buf[i];
			 if (linebuf[writepos]=='\n') {
			  decodeCommand(linebuf,serport);
			  writepos=0;
			  }
			 else {
			  writepos++;
			 }
			 if (writepos>=256) writepos=0;
			}
			ib.read_cnt=0;
		}
		

		if (isEOF(sock_fd, &ib)) {
					rprintf("Server died, exitting.\n");
					exit(0);
		}
		
	}

	return 0;
}

