/* NeuroServer
 *
 * Version 0.72 by Rudi Cilibrasi (cilibrar@ofb.net)
 *
 * A TCP/IP server to allow ubiquitous EEG data collection and analysis.
 *
 * This server translates TCP EDF-oriented protocol messages into lower
 * level serial-port messages to communicate with EEG hardware.
 *
 * At this point, it only works with ModularEEG using p2 format firmware.
 *
 */

#include <assert.h>
#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <malloc.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include <neurod.h>
#include <openedf.h>

static struct EDFDecodedConfig p2Cfg = {
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

struct InputBuffer {
	unsigned char inbuf[MAXLINELENGTH];
	unsigned int inbufCount;
};

struct ClientRecord {
	int fd;
	unsigned int isController;
	unsigned int isReceiving;
	unsigned int isDisconnected;
	struct InputBuffer in;
};

void removeClient(int which);

void setnonblocking(int sock)
{
	int opts;

	opts = fcntl(sock,F_GETFL);
	if (opts < 0) {
		perror("fcntl(F_GETFL)");
		exit(1);
	}
	opts = (opts | O_NONBLOCK);
	if (fcntl(sock,F_SETFL,opts) < 0) {
		perror("fcntl(F_SETFL)");
		exit(1);
	}
	return;
}

void printFullEDFHeader(int fd)
{
	printf("Sending EDF header\n");
	writeEDF(fd, &current);
}

void watchForP2(unsigned char nextByte)
{
	static struct InputBuffer ib;
	unsigned short *s;
	unsigned int vals[2];
	eatCharacter(&ib, nextByte);
	if (ib.inbufCount < 17) return;
	if (ib.inbuf[0] != 0xa5) return;
	if (ib.inbuf[1] != 0x5a) return;
	if (ib.inbuf[2] != 2) return;
	// Assume 2 channels.  TODO: Fix this to work with more
	s = (unsigned short *) (&ib.inbuf[4]);
	vals[0] = ntohs(s[0]);
	vals[1] = ntohs(s[1]);
	// TODO: figure out why this line is necessary with p2 firmware!
	if (vals[0] < 0 || vals[0] > 1023 || vals[1] < 0 || vals[1] > 1023)
		return;
//	printf("The packet counter is %03x\n", ib.inbuf[3]);
	handleSamples(2, vals);
}

struct ClientRecord clients[MAX_CLIENTS];
unsigned int clientCount = 0;
int highSock;
int currentClient;
int pendingRemovals = 0;

const char *getRecorderName(void)
{
	static char namebuf[81];
	sprintf(namebuf, "Ms. Default Recorder");
	return namebuf;
}

void updateHighSock(int val)
{
	if (highSock < val)
		highSock = val;
}

unsigned int isNewLine(char c)
{
	return c == '\r' || c == '\n';
}

void eatCharacter(struct InputBuffer *ib, unsigned char nextChar)
{
	memmove(ib->inbuf, ib->inbuf+1, MAXLINELENGTH-1);
	ib->inbuf[MAXLINELENGTH-1] = nextChar;
	if (ib->inbufCount < MAXLINELENGTH)
		ib->inbufCount += 1;
}

unsigned int doesMatchCommandNL(const struct InputBuffer *ib, const char *str)
{
	unsigned char c;
	int i, len;
	c = ib->inbuf[MAXLINELENGTH-1];
	if (c != '\n' && c != '\r')
		return 0;
	len = strlen(str);
	if (ib->inbufCount < len + 1)
		return 0;
	for (i = 0; i < len; ++i)
		if (tolower(ib->inbuf[MAXLINELENGTH-len-1+i]) != tolower(str[i]))
			return 0;
	return 1;
}

/*
 *      * 'open_port()' - Open serial port 1.
 *      *
 *      * Returns the file descriptor on success or -1 on error.
 *      */

    int
    open_port(void)
    {
      int fd; /* File descriptor for the port */


      fd = open("/dev/ttyS0", O_RDWR | O_NOCTTY | O_NDELAY);
      if (fd == -1)
      {
       /*
 * 	  * Could not open the port.
 * 		*/

	perror("open_port: Unable to open /dev/ttyS0 - ");
      }
      else
				fcntl(fd, F_SETFL, FNDELAY);


      return (fd);
    }

void set_port_options(int fd)
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

    cfsetispeed(&options, B57600);
    cfsetospeed(&options, B57600);

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
}

/* Call this function with no newlines at the end of msg */
void sendResponse(int toWhom, int respCode, const char *msg)
{
	char buf[128];
	sprintf(buf, "%03d %s\r\n", respCode, msg);
	write(clients[toWhom].fd, buf, strlen(buf));
}

void sendResponseBadRequest(int toWhom)
{
	sendResponse(toWhom, 400, "BAD REQUEST");
}

void sendResponseOK(int toWhom)
{
	sendResponse(toWhom, 200, "OK");
}

void handler_SIGPIPE(int whatnot)
{
	clients[currentClient].isDisconnected = 1;
	pendingRemovals = 1;
}

int main(int argc, char **argv)
{
	int retval;
	int serfd;
	int sockfd;
	int controllingIndex = -1;
	fd_set toRead;
	struct sockaddr_in localAddress;
	char c;
	int i;
	signal(SIGPIPE, handler_SIGPIPE);
	/* Initialize default configuration to p2Cfg */
	makeREDFConfig(&current, &p2Cfg);
	/* Open serial port */
	printf("Opening serial port\n");
	serfd = open_port();
	updateHighSock(serfd);
	/* Set 57600-8-N-1 */
	set_port_options(serfd);
	/* Create a TCP socket */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	updateHighSock(sockfd);
	assert(sockfd >= 0 && "Cannot create socket!");
	/* Bind to all addresses at DEFAULT_PORT */
	localAddress.sin_family = AF_INET;
	localAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	localAddress.sin_port = htons(DEFAULT_PORT);
	retval = bind(sockfd, (struct sockaddr *) &localAddress, sizeof(localAddress));
	assert(retval == 0 && "socket bind failed");
	printf("Binding to port %d\n", DEFAULT_PORT);
	retval = listen(sockfd, MAX_CLIENTS);
	assert(retval == 0 && "socket listen failed");
	// setnonblocking(sockfd);  // TODO: two-phase asynchronous connect
	printf("Ready for incoming TCP connections\n");
	for (;;) {
		FD_ZERO(&toRead);
		FD_SET(serfd, &toRead);
		FD_SET(sockfd, &toRead);
		for (i = 0; i < clientCount; ++i)
			FD_SET(clients[i].fd, &toRead);
		retval = select(highSock+1, &toRead, 0, 0, NULL);
		if (retval < 0) {
			perror("select");
			exit(1);
		}
		if (retval == 0) {
			continue;
		}
		if (FD_ISSET(serfd, &toRead)) {
			retval = read(serfd, &c, 1);
			if (retval == 1) {
				watchForP2(c);
			}
		}

		if (FD_ISSET(sockfd, &toRead) && clientCount < MAX_CLIENTS) {
			clients[clientCount].fd = accept(sockfd, NULL, NULL);
			write(clients[clientCount].fd, BANNERSTR, strlen(BANNERSTR));
			printf("Got new connection, there are now %d clients\n", clientCount+1);
			if (clients[clientCount].fd < 0) {
				perror("accept");
				continue;
			}
			updateHighSock(clients[clientCount].fd);
			setnonblocking(clients[clientCount].fd);
			if (controllingIndex == -1)
				controllingIndex = clientCount;
			clients[clientCount].isController = (controllingIndex == clientCount);
			clients[clientCount].isReceiving = 0;
			clients[clientCount].isDisconnected = 0;
			clients[clientCount].in.inbufCount = 0;
			clientCount += 1;
		}
		if (pendingRemovals) {
			for (i = 0; i < clientCount; ++i) {
				if (clients[i].isDisconnected)
					removeClient(i);
			}
			pendingRemovals = 0;
		}
		for (i = 0; i < clientCount; ++i) {
			if (FD_ISSET(clients[i].fd, &toRead)) {
				retval = read(clients[i].fd, &c, 1);
				if (retval == 1) {
					eatCharacter(&clients[i].in, c);
					if (isNewLine(c)) {
						if (isNewLine(clients[i].in.inbuf[MAXLINELENGTH-2])) {
							continue;  // ignore blank lines
						}
						if (doesMatchCommandNL(&clients[i].in, "GetEDFHeader")) {
							sendResponseOK(i);
							printFullEDFHeader(clients[i].fd);
							continue;
						}
						if (doesMatchCommandNL(&clients[i].in, "GetConnStatus")) {
							char *str;
							sendResponseOK(i);
							str = clients[i].isController ? "controller" : "observer";
							write(clients[i].fd, str, strlen(str));
							continue;
						}
						if (doesMatchCommandNL(&clients[i].in, "SendSamples")) {
							sendResponseOK(i);
							clients[i].isReceiving = 1;
							continue;
						}
						sendResponseBadRequest(i);
					}
				}
			}
		}
	}
	printf("Out of main loop\n");
	close(serfd);
	return 0;
}

void removeClient(int which)
{
	printf("Lost connection with client %d\n", which);
	if (which != clientCount - 1) {
		memmove(clients+which, clients+which+1, 
			sizeof(clients[0]) * (clientCount - which - 1));
	}
	clientCount -= 1;
}

void handleSamples(unsigned int channelCount, const unsigned int *values)
{
//	printf("Got a %d-channel sample set: %08x, %08x\n", channelCount, values[0], values[1]);
	int i;
	char sbuf[4096];
	char *ptr;
	int len;
	ptr = sbuf;
	for (i = 0; i < channelCount; ++i) {
		ptr += sprintf(ptr, "%d ", values[i]);
		assert(values[i] >= 0 && values[i] < 1024);
	}
	len = strlen(sbuf);
	sbuf[len-1] = '\n';
	for (i = 0; i < clientCount; ++i) {
		if (!clients[i].isReceiving)
			continue;
		currentClient = i;
		write(clients[i].fd, sbuf, len);
	}
}
