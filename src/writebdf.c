/*! \brief Program to save data from the neuroserver in a file

 \author Rudi Cilibrasi, Moritz von Buttlar
 \todo add CSV output (excel, Scilab, gnuplot etc. direct import)

 This program is used to connect to the neuroserver and receive
 an eeg data stream. It saves this data in a file using the
 European Data Format (EDF). There's also an option to use the
 24bit extended format like it is specified and used by BioSemi (also called
 BDF). This is useful for e.g. 24bit DC coupled eeg devices. 
 The 24bit mode is also auto-detected if the data format field starts with "24".
 For the file data binary numbers are used.

 The program writes data blockwise: it opens the file, writes one data
 record, then closes the file (in the write_buffers function)
 

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
                
#include <assert.h>
#include <openedf.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pctimer.h>
#include <ctype.h>
#include <nsutil.h>
#include <nsnet.h>

const char *helpText =
"writeedf   v 0.35 by Rudi Cilibrasi, Moritz von Buttlar\n"
"\n"
"Usage:  writeedf [options] <filename>\n"
"\n"
"        -p port          Port number to use (default 8336)\n"
"        -n hostname      Host name of the NeuroCaster server to connect to\n"
"                         (this defaults to 'localhost')\n"
"        -s <num>         Only record this many seconds.\n"
"                         (default is to record until interrupted)\n"
"        -e <intnum>      Integer ID specifying which EEG to log (default: 0)\n"
"        -b               use bdf format (same as edf, only 24 bits/sample) \n"
"        -c               write as CSV array (can be imported to Excel, Scilab,etc.)"
"The filename specifies the new EDF file to create.\n"
;

#define MINLINELENGTH 4
#define DELIMS " \r\n"

/*! \brief Command line options will be decoded into this structure
*/
struct Options {
	char hostname[MAXLEN];  //!< neuroserver host name 
	unsigned short port;	//!< neuroserver port number 
	char filename[MAXLEN];  //!< filename for data file 
	int eegNum;		//!< number of eeg channel on ns
	int isFilenameSet;	//!< flag for filename 
	int isBdf;		//!< flag for 24bit data format (bdf)	
	int isArray;		//!< flag for CSV (comma seperated value) format 
	int isLimittedTime;	//!< flag for time limit 
	double seconds;		//!< number of seconds this application should run
};
 

struct EDFDecodedConfig cfg;	//!< EDF header information block
static struct OutputBuffer ob;  //!< network output buffer
struct InputBuffer ib;		//!< network input buffer
char lineBuf[MAXLEN];		//!< buffer for ASCII data frames
int blockCounter = 0;		//!< number of data blocks written

char *dataarray;		//!< pointer for data block
long samples[MAXCHANNELS];	//!< channel sample data

size_t sampleSize;		//!< amount of memory for one sample (16 or 24 bits)
unsigned int frameCount;	//!< counter for received data frames
struct Options opts;		//!< place to store command line options

/*! \brief reset frame counter
*/
void resetSampleBuffer()
{
	frameCount = 0;
}

/*! \brief writes sample data into data buffer in the right order
 */
void writeSampleBuffer(unsigned int Channel,unsigned int SampleNr, char Byte0, char Byte1, char Byte2) {
 if (opts.isBdf) {
  dataarray[Channel*cfg.chan[0].sampleCount*sampleSize+SampleNr*sampleSize]=Byte0;
  dataarray[Channel*cfg.chan[0].sampleCount*sampleSize+SampleNr*sampleSize+1]=Byte1;
  dataarray[Channel*cfg.chan[0].sampleCount*sampleSize+SampleNr*sampleSize+2]=Byte2;
 }
 else {
  dataarray[Channel*cfg.chan[0].sampleCount*sampleSize+SampleNr*sampleSize]=Byte1;
  dataarray[Channel*cfg.chan[0].sampleCount*sampleSize+SampleNr*sampleSize+1]=Byte2;
 }
}

/*! \brief allocates memory for data buffer
*/
void initSampleBuffer()
{
	if (opts.isBdf) 
	 sampleSize=sizeof(unsigned char[3]);	//!< get 3 bytes/sample (BDF Standard)
	else 
 	 sampleSize=sizeof(unsigned char[2]); 	//!< get 2 bytes/sample (EDF Standard)	
        dataarray = (unsigned char *) malloc(cfg.hdr.dataRecordChannels*
                     sampleSize*cfg.chan[0].sampleCount*sizeof(unsigned char) ); //!< allocate one big array of bytes
	resetSampleBuffer();
	assert(dataarray != NULL && "Cannot allocate memory!");
}


/*! \brief writes data chunk to file
 */
void writeBufferToFile()
{
	FILE *fp;
	fp = fopen(opts.filename, "a");
	rprintf("File opened for block write, frameCount: %d \n",frameCount);
	assert(fp != NULL && "Cannot open file!");	
	/// goto end of file
	fseek(fp, 0, SEEK_END);
	/// block write sample data
	fwrite(dataarray, sizeof(unsigned char) , cfg.hdr.dataRecordChannels*frameCount*sampleSize, fp);
	fclose(fp);
	rprintf("Wrote block %d\n", blockCounter);
	blockCounter += 1;
	resetSampleBuffer();
}


/*! \brief copy data to sample buffer, write buffer to file if it is full
 */
void handleSamples(int packetCounter, int channels)
{
	static int lastPacketCounter = 0;
	int i,j;
  	unsigned char c1,c2,c3;
	if (lastPacketCounter + 1 != packetCounter && packetCounter != 0 && lastPacketCounter != 0) {
		rprintf("May have missed packet: got packetCounters %d and %d\n", lastPacketCounter, packetCounter);
	}
	lastPacketCounter = packetCounter;
	for (i = 3; i < channels+3; ++i) {
//         rprintf("Writing data from Channel: %d Value: %d Sample: %d to buffer \n",i,samples[i],frameCount);

// Now we have to convert a 4 byte 2s complement long number into a 3 or 2 byte 2s complement number.
// This is done by removing the most significant 8 or 16 bits and adding the sign bit to the most significant
// bit of the remaining number. The result is saved in three unsigned char bytes.
         if (opts.isBdf) {
           c1=((samples[i])  & (0xff));             // get lowest byte
           c2=((samples[i]>>8) & (0xff));
           c3=((samples[i]>>16) & (0xff));          // get highest byte
       //   if ( ((samples[i]>>24) & (0x80))==(0x80)) c3=c3+(0x80); // set highest bit (number is negative)
         }
         else {
           c1=((samples[i])  & (0xff));             // get lowest byte
           c2=((samples[i]>>8) & (0xff));
           if ((samples[i]>>16) & (0x80)) c2=c2+(0x80); // set highest bit (number is negative)
         }
         
         rprintf(" Sample: %d c1: %d c2: %d c3: %d \n",samples[i],c1,c2,c3);
         writeSampleBuffer(i-3,frameCount,c1,c2,c3);	
   	}
   	frameCount += 1;
        if (frameCount==cfg.chan[0].sampleCount) {
         writeBufferToFile();
	}
}




/*! \brief check, if data is a number
 */
 
int isANumber(const char *str) {
	int i;
	for (i = 0; str[i]; ++i)
		if ( (!isdigit(str[i])) && (!(str[i]=='-')) )
			return 0;
	return 1;
}

/*! \brief action to take if server died
 */
void serverDied(void)
{
	rprintf("Server died!\n");
	exit(1);
}



int main(int argc, char **argv)
{
	char EDFPacket[MAXHEADERLEN];	
	char cmdbuf[80];
	int EDFLen = MAXHEADERLEN;
	FILE *fp;
	fd_set toread;
	int linePos = 0;
	int i;
	double tStart,tEnde;
	sock_t sock_fd;
	int retval;

	strcpy(opts.hostname, DEFAULTHOST);
	opts.port = DEFAULTPORT;
	opts.isBdf=0;
	opts.seconds=100000;
	for (i = 1; i < argc; ++i) {
		char *opt = argv[i];
		if (opt[0] == '-') {
			switch (opt[1]) {
				case 'h':
					printf("%s", helpText);
					exit(0);
					break;
				case 's':
					opts.seconds = atof(argv[i+1]);
					opts.isLimittedTime = 1;
					i += 1;
					break;
				case 'e':
					opts.eegNum = atoi(argv[i+1]);
					i += 1;
					break;
				case 'n':
					strcpy(opts.hostname, argv[i+1]);
					i += 1;
					break;
				case 'p':
					opts.port = atoi(argv[i+1]);
					i += 1;
					break;
				case 'b':
					opts.isBdf=1;
					break;					
				case 'c':
					opts.isArray=1;
					break;					
				}
			}
		else {
			if (opts.isFilenameSet) {
				fprintf(stderr, "Error: only one edf file allowed: %s and %s\n",
						opts.filename, argv[i]);
				exit(1);
			}
			strcpy(opts.filename, argv[i]);
			opts.isFilenameSet = 1;
		}
	}
/// check filename
	if (!opts.isFilenameSet) {
		fprintf(stderr, "Must supply the filename of the EDF to write.\n");
		exit(1);
	}
/// open file	
	fp = fopen(opts.filename, "wb");
	assert(fp != NULL && "Cannot open file!");

/// start networking
	rinitNetworking();
	sock_fd = rsocket();
	if (sock_fd < 0) {
		perror("socket");
		rexit(1);
	}

	retval = rconnectName(sock_fd, opts.hostname, opts.port);
	if (retval != 0) {
		rprintf("connect error\n");
		rexit(1);
	}

	rprintf("Socket connected.\n");
	fflush(stdout);

/// configure neuroserver connection
	writeString(sock_fd, "display\n", &ob);
	getOK(sock_fd, &ib);
	rprintf("Finished display, doing getheader.\n");
	sprintf(cmdbuf, "getheader %d\n", opts.eegNum);
	writeString(sock_fd, cmdbuf, &ob);
	getOK(sock_fd, &ib);
	if (isEOF(sock_fd, &ib))
		serverDied();
	EDFLen = readline(sock_fd, EDFPacket, sizeof(EDFPacket), &ib);
	rprintf("Got EDF Header <%s>\n", EDFPacket);
        
/// now decode EDF header        
	readEDFString(&cfg, EDFPacket, EDFLen);
/// check for 24bit BDF data format
	if ( (cfg.hdr.dataFormat[0]=='2') && (cfg.hdr.dataFormat[1]=='4') ) {
	 opts.isBdf=1;
	 rprintf("24bit data format auto detected \n");
	}
	rprintf("Header Record bytes: %d \n",cfg.hdr.headerRecordBytes);
	
/// write header to data file (if not in CSV mode)
	if (opts.isArray==0) fwrite(EDFPacket, 1, cfg.hdr.headerRecordBytes, fp);
	fclose(fp);
	
	initSampleBuffer();
	rprintf("Now sending watch command \n");	
	sprintf(cmdbuf, "watch %d\n", opts.eegNum);
	writeString(sock_fd, cmdbuf, &ob);
	getOK(sock_fd, &ib);
	tStart = pctimer();
	FD_ZERO(&toread);
	FD_SET(sock_fd, &toread);
	rprintf("Starting data read loop \n");
	while ( (tStart+opts.seconds) > pctimer() ) {
			char *cur;
			int curParam = 0;
			int devNum, packetCounter=0, channels=0;
			rselect(sock_fd+1, &toread, NULL, NULL);
			linePos = readline(sock_fd, lineBuf, sizeof(EDFPacket), &ib);
			if (isEOF(sock_fd, &ib)) {
			 rprintf("EOF sock_fd \n");
			 break;
			}
			if (linePos < MINLINELENGTH)
				continue;
			if (lineBuf[0] != '!')
				continue;
			curParam=0;	
			for (cur = strtok(lineBuf, DELIMS); cur ; cur = strtok(NULL, DELIMS)) {
				if (isANumber(cur)){
				 samples[curParam]=atol(cur);
				 rprintf("Nr.: %d \t Wert:  %d \t \n",curParam,samples[curParam]);
				 curParam++;
				}
			}
			/// <devicenum> <packetcounter> <channels> data0 data1 data2 ...
			if (curParam < 3)
				continue;
			packetCounter=samples[1];	
			channels=samples[2];	
			rprintf("got samples, channels: %d \t packetCounter: %d \n",channels,packetCounter);
			handleSamples(packetCounter,channels);
		}
		
		
	rprintf("Ending data read loop \n");

	return 0;
}
