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
                
#ifndef __OPENEDF_H
#define __OPENEDF_H

#include <stdio.h>

#include <nsnet.h>

#define MAXCHANNELS 32
#define BYTESPERSAMPLE 2
#define MAXHEADERLEN ((MAXCHANNELS+1) * 256)

#pragma pack(1)

struct EDFPackedHeader {
	char dataFormat[8];			// identification code
	char localPatient[80];			// local subject identification [ASCII]
	char localRecorder[80];			// local recording identification [ASCII]
	char recordingStartDate[8];	        // dd.mm.yy local start date of recording [ASCII]
	char recordingStartTime[8];		// hh.mm.ss local start time of recording [ASCII]
	char headerRecordBytes[8];		// number of bytes in header record [ASCII]
	char manufacturerID[44];		// version/data format/manufact. [ASCII]
	char dataRecordCount[8];		// number of data records, -1 if unknown
	char dataRecordSeconds[8];		// duration of a data record in seconds [ASCII] z.B. 1
	char dataRecordChannels[4];		// number of recording channels in data record
};

struct EDFPackedChannelHeader {
	char label[16];				// channel label
	char transducer[80];			// type of transducer
	char dimUnit[8];			// physical dimension of channel, e.g. uV 
	char physMin[8];			// minimal physical value (in above units)
	char physMax[8];			// maximal physical value
	char digiMin[8];			// minimal digital value
	char digiMax[8];			// max. digital value
	char prefiltering[80];			// pre-filtering description
	char sampleCount[8];			// number of samples in each record/data chunk
	char reserved[32];			// reserved for future extensions
};

#pragma pack(4)

struct EDFDecodedHeader {
	int headerRecordBytes;			// size of header in bytes
	int dataRecordCount;			// number of data records/chunks
	int dataRecordChannels;			// number of channels
	char dataFormat[9];
	char localPatient[81];
	char localRecorder[81];
	char recordingStartDate[9];
	char recordingStartTime[9];
	char manufacturerID[45];
	double dataRecordSeconds;		// duration of a data record in seconds
};

struct EDFDecodedChannelHeader {
	int sampleCount;
	double physMin, physMax;		// physical maxima/minima
	double digiMin, digiMax;  		// digital maxima/minima -->gain
	char label[17];
	char transducer[81];
	char dimUnit[9];
	char prefiltering[81];
	char reserved[33];
};

struct EDFDecodedConfig {
	struct EDFDecodedHeader hdr;
	struct EDFDecodedChannelHeader chan[MAXCHANNELS];
};

struct EDFInputIterator {
	struct EDFDecodedConfig cfg;
	int dataRecordNum;
	int sampleNum;
	char *dataRecord;
};

struct EDFInputIterator *newEDFInputIterator(const struct EDFDecodedConfig *cfg);
int stepEDFInputIterator(struct EDFInputIterator *edfi);
int fetchSamples(const struct EDFInputIterator *edfi, short *samples, FILE *fp);
void freeEDFInputIterator(struct EDFInputIterator *edfi);

int EDFUnpackInt(const char *inp, int fieldLen);
double EDFUnpackDouble(const char *inp, int fieldLen);
const char *EDFUnpackString(const char *inp, int fieldLen);

int EDFEncodePacket(char *target, const struct EDFDecodedConfig *cfg);

int writeEDF(int fd, const struct EDFDecodedConfig *cfg);
int readEDF(int fd, struct EDFDecodedConfig *cfg);
int writeEDFString(const struct EDFDecodedConfig *cfg, char *buf, int *buflen);
int readEDFString(struct EDFDecodedConfig *cfg, const char *buf, int len);

int setEDFHeaderBytes(struct EDFDecodedConfig *cfg);
int getDataRecordChunkSize(const struct EDFDecodedConfig *cfg);

double getSamplesPerSecond(const struct EDFDecodedConfig *cfg, int whichChan);
double getSecondsPerSample(const struct EDFDecodedConfig *cfg, int whichChan);

/* Realtime EDF (REDF) is a restricted type of EDF for the network */
int isValidREDF(const struct EDFDecodedConfig *cfg);
int makeREDFConfig(struct EDFDecodedConfig *result, const struct EDFDecodedConfig *source);

/* Returns a text description of the last error */
const char *getLastError(void);

const char *getDMY(void);
const char *getHMS(void);

#endif

