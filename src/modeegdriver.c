#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <nsnet.h>
#include <nsutil.h>
#include <nsser.h>
#include <openedf.h>

/* This is the maximum size of a protocol packet */
#define PROTOWINDOW 24

#define MAXPACKETSIZE 17

char buf[PROTOWINDOW];
sock_t sock_fd;
int bufCount;
static int failCount = 0;
static struct OutputBuffer ob;
static struct InputBuffer ib;

int hasMatchedProtocol;
int isP2, isP3;
int isValidPacket(unsigned short nchan, unsigned short *samples);
int doesMatchP3(unsigned char c, unsigned short *vals,int *nsamples);
int doesMatchP2(unsigned char c, unsigned short *vals,int *nsamples);
int mGetOK(sock_t fd, struct InputBuffer *ib);

int mGetOK(sock_t fd, struct InputBuffer *ib)
{
	return 0;
}

static struct EDFDecodedConfig modEEGCfg = {
		{ 0,   // header bytes, to be set later
			-1,  // data record count
			2,   // channels
			"0", // data format
			"Mr. Default Patient",
			"Ms. Default Recorder",
			"date",
			"time",
			"manufacturer",
			1.0,  // 1 second per data record
		}, {
				{
					256, // samples per second
					-512, 512, // physical range
					0, 1023,   // digital range
					"electrode 0", // label
					"metal electrode",  // transducer
					"uV",  // physical unit dimensions
					"LP:59Hz", // prefiltering
					""         // reserved
				},
				{
					256, -512, 512, 0, 1023, 
					"electrode 1", "metal electrode", "uV", "LP:59Hz", ""
				}
		}
	};

static struct EDFDecodedConfig current;

int isValidPacket(unsigned short chan, unsigned short *samples)
{
	int i;
	if (chan != 2 && chan != 4 && chan != 6) return 0;
	for (i = 0; i < chan; i++)
		if (samples[i] > 1023) return 0;
	return 1;
}

void resetBuffer(void)
{
	bufCount = 0;
}

void gobbleChars(int howMany)
{
	assert(howMany <= bufCount);
	if (bufCount > howMany)
		memmove(buf, buf+howMany, bufCount-howMany);
	bufCount -= howMany;
}

void eatCharacter(unsigned char c)
{
	int oldHasMatched;
	unsigned short vals[MAXCHANNELS];
	int ns;
	int didMatch = 0;
	oldHasMatched = hasMatchedProtocol;
	if (bufCount == PROTOWINDOW - 1)
		memmove(buf, buf+1, PROTOWINDOW - 1);
	buf[bufCount] = c;
	if (bufCount < PROTOWINDOW - 1)
		bufCount += 1;
	if (hasMatchedProtocol) {
		if (isP2)
			didMatch = doesMatchP2(c, vals, &ns);
		if (isP3)
			didMatch = doesMatchP3(c, vals, &ns);
	} else {
		if ( (didMatch = doesMatchP2(c, vals, &ns)) ) {
			hasMatchedProtocol = 1;
			isP2 = 1;
		} else {
			if ( (didMatch = doesMatchP3(c, vals, &ns)) ) {
				hasMatchedProtocol = 1;
				isP3 = 1;
			}
		}
	}
	if (didMatch) {
		char *pstr = "xx";
		if (!isValidPacket(ns, vals)) {
			rprintf("Warning: invalid serial packet ahead!\n");
		}
		if (isP2)
			pstr = "P2";
		if (isP3)
			pstr = "P3";
			
		failCount = 0;
	
//		printf("Got %s packet with %d values: ", pstr, ns);
//		for (i = 0; i < ns; ++i)
//			printf("%d ", vals[i]);
//		printf("\n");
	}
	else {
		failCount += 1;
		if (failCount % PROTOWINDOW == 0) {
			rprintf("Serial packet sync error -- missed window.\n");
		}
	}
}

void handleSamples(int packetCounter, int chan, unsigned short *vals)
{
	char buf[MAXLEN];
	int bufPos = 0;
	int i;
	if (chan > current.hdr.dataRecordChannels)
		chan = current.hdr.dataRecordChannels;
	bufPos += sprintf(buf+bufPos, "! %d %d", packetCounter, chan);
	for (i = 0; i < chan; ++i)
		bufPos += sprintf(buf+bufPos, " %d", vals[i]);
	bufPos += sprintf(buf+bufPos, "\r\n");
	writeString(sock_fd, buf, &ob);
	mGetOK(sock_fd, &ib);
}

int doesMatchP3(unsigned char c,  unsigned short *vals,int *nsamples)
{
	static int i = 0;
	static int j = 0;
	static int packetCounter = 0;
	static int auxChan = 0;
	static short samples[MAXCHANNELS];
	static int needsHeader = 1;
	static int needsAux;
	if (needsHeader) {
		if (c & 0x80) goto resetMachine;
		packetCounter = c >> 1;
		auxChan = c & 1;
		needsHeader = 0;
		needsAux = 1;
		return 0;
	}
	if (needsAux) {
		if (c & 0x80) goto resetMachine;
		auxChan += (int) c * 2;
		needsAux = 0;
		return 0;
	}
	if (2*i < MAXCHANNELS) {
       switch (j) {
           case 0: samples[2*i] = (int) c; break;
           case 1: samples[2*i+1] = (int) c; break;
           case 2:
               samples[2*i] += (int)   (c & 0x70) << 3;
               samples[2*i+1] += (int) (c & 0x07) << 7;
               i += 1;
               break;
       }
	}
	j = (j+1) % 3;
	if (c & 0x80) {
		if (j == 0) {
			memcpy(vals, samples, sizeof(vals[0]) * i * 2);
			*nsamples = i * 2;
			i = 0;
			j = 0;
			needsHeader = 1;
			handleSamples(packetCounter, *nsamples, vals);
			return 1;
		}
		else {
			rprintf("P3 sync error:i=%d,j=%d,c=%d.\n",i,j,c);
		}
		goto resetMachine;
	}
	return 0;

	resetMachine:
		i = 0;
		j = 0;
		needsHeader = 1;
	return 0;
}

int doesMatchP2(unsigned char c, unsigned short *vals,int *nsamples)
{
	enum P2State { waitingA5, waiting5A, waitingChannel, waitingData };
	static int packetCounter = 0;
	static int chan = 0;
	static int i = 0;
	static enum P2State state = waitingA5;
	unsigned short s[MAXCHANNELS];
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
				}
			}
			break;

		case waiting5A:
			if (c == 0x5a)
				state = waitingChannel;
			else
				state = waitingA5;
			break;

		case waitingChannel:
			if (c == 2 || c == 4 || c == 6) {
				chan = c;
				i = 0;
				state = waitingData;
			}
			else
				state = waitingA5;
			break;

		case waitingData:
			if ((i%2) == 0)
				s[i/2] = c;
			else
				s[i/2] += (int) c * 256;
			i += 1;
			if (i == chan*2) {
				*nsamples = chan;
				memcpy(vals, s, chan * sizeof(s[0]));
				handleSamples(packetCounter, *nsamples, vals);
				packetCounter = (packetCounter + 1) % 128;
				state = waitingA5;
				return 1;
			}
			break;
			default:
				rprintf("Error: unknown state in P2!\n");
				rexit(0);
			}
	return 0;
}

int main(int argc, char **argv)
{
	char responseBuf[MAXLEN];
	int retval;
	ser_t serport;
	char EDFPacket[MAXHEADERLEN];
	int EDFLen = MAXHEADERLEN;
	char smallbuf[PROTOWINDOW];
	char *hostname = DEFAULTHOST;
	unsigned short portno = DEFAULTPORT;
	struct timeval when;

	rinitNetworking();

	if (argc > 1)
		hostname = argv[1];
	if (argc > 2)
		portno = atoi(argv[2]);
	rprintf("The modeeg test driver has started.\n");
	
	makeREDFConfig(&current, &modEEGCfg);
	writeEDFString(&current, EDFPacket, &EDFLen);


	sock_fd = rsocket();
	if (sock_fd < 0) {
		perror("socket");
		rexit(1);
	}
	setblocking(sock_fd);

	retval = rconnectName(sock_fd, hostname, portno);
	if (retval != 0) {
		rprintf("connect error\n");
		rexit(1);
	}
	rprintf("Socket connected.\n");
	fflush(stdout);

	serport = openSerialPort(DEVICENAME);
	rprintf("Serial port %s opened.\n", DEVICENAME);
	
	writeString(sock_fd, "eeg\n", &ob);
	mGetOK(sock_fd, &ib);
	writeString(sock_fd, "setheader ", &ob);
	writeBytes(sock_fd, EDFPacket, EDFLen, &ob);
	writeString(sock_fd, "\n", &ob);
	mGetOK(sock_fd, &ib);
	updateMaxFd(sock_fd);
#ifndef __MINGW32__
	updateMaxFd(serport);
#endif
	rprintf("Polling at %d Hertz or %d usec\n", modEEGCfg.chan[0].sampleCount, when.tv_usec);
	for (;;) {
		int i, readSerBytes;
		int retval;
		fd_set toread;
		when.tv_sec = 0;
		when.tv_usec = (1000000L / modEEGCfg.chan[0].sampleCount);
		FD_ZERO(&toread);
		FD_SET(sock_fd, &toread);
#ifndef __MINGW32__
		FD_SET(serport, &toread);
#endif
		retval = rselect_timed(max_fd, &toread, NULL, NULL, &when);
		readSerBytes = readSerial(serport, smallbuf, PROTOWINDOW);
		if (readSerBytes < 0) {
			rprintf("Serial error.\n");
		}
		if (readSerBytes > 0) {
			for (i = 0; i < readSerBytes; ++i)
				eatCharacter(smallbuf[i]);
		}
		if (FD_ISSET(sock_fd, &toread)) {
			my_read(sock_fd, responseBuf, MAXLEN, &ib);
		}
		if (isEOF(sock_fd, &ib)) {
					rprintf("Server died, exitting.\n");
					exit(0);
		}
	}

	return 0;
}

