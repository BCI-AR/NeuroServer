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
#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <malloc.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include <neurod.h>

struct HardwareSettings {
	/* Global settings */
	int channelCount;
	int sampleRate;
};

struct HardwareSettings current = { 2, 256 };

struct InputBuffer {
	unsigned char inbuf[MAXLINELENGTH];
	unsigned int inbufCount;
};

struct ClientRecord {
	int fd;
	unsigned int isController;
	unsigned int isReceiving;
	struct InputBuffer in;
};

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
	write(fd, printEDFHeader(), 256);
	write(fd, printChannelBlock(), current.channelCount * 256);
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
//	printf("The packet counter is %03x\n", ib.inbuf[3]);
	handleSamples(2, vals);
}

struct ClientRecord clients[MAX_CLIENTS];
unsigned int clientCount = 0;
int highSock;

const char *getRecorderName(void)
{
	static char namebuf[81];
	sprintf(namebuf, "Ms. Default Recorder");
	return namebuf;
}

const char *getDMY(void)
{
	static char buf[81];
	time_t t;
	struct tm *it;
	time(&t);
	it = localtime(&t);
	sprintf(buf, "%02d.%02d.%02d", it->tm_mday, it->tm_mon+1, it->tm_year % 100);
	return buf;
}

const char *getHMS(void)
{
	static char buf[81];
	time_t t;
	struct tm *it;
	time(&t);
	it = localtime(&t);
	sprintf(buf, "%02d.%02d.%02d", it->tm_hour, it->tm_min, it->tm_sec);
	return buf;
}

const char *getPatientName(void)
{
	static char namebuf[81];
	sprintf(namebuf, "Mr. Default Patient");
	return namebuf;
}

const char *printChannelBlock()
{
	static char *buf = NULL;
#if 0
	if (buf == NULL) {
		char *ptr;
		int i;
		buf = calloc(1, 1+256*current.channelCount);
		ptr = buf;
		for (i = 0; i < 2; ++i) {
			char namebuf[80];
			sprintf(namebuf, "electrode%02d", i);
			ptr += sprintf(ptr, "%- 16.16s", namebuf);
		}
		for (i = 0; i < 2; ++i) ptr += sprintf(ptr, "%- 80.80s", "metal electrode");
		for (i = 0; i < 2; ++i) ptr += sprintf(ptr, "%- 8.8s", "uV");
		for (i = 0; i < 2; ++i) ptr += sprintf(ptr, "%- 8.8s", "-500"); // physmin
		for (i = 0; i < 2; ++i) ptr += sprintf(ptr, "%- 8.8s", "500");  // physmax
		for (i = 0; i < 2; ++i) ptr += sprintf(ptr, "%- 8.8s", "-512");  // digmin
		for (i = 0; i < 2; ++i) ptr += sprintf(ptr, "%- 8.8s", "512");   // digmax
		for (i = 0; i < 2; ++i) ptr += sprintf(ptr, "%- 80.80s", 
					"LP:59Hz");   // prefiltering by a 59Hz lowpass filter
		for (i = 0; i < 2; ++i) ptr += sprintf(ptr, "%- 8.8s", "1");
		for (i = 0; i < 2; ++i) ptr += sprintf(ptr, "%- 32.32s", "");
	}
#endif
	return buf;
}

const char *printEDFHeader()
{
	static char hbuf[257];
#if 0
	char strDur[128], strChan[16], strHeaderBytes[16];
	hbuf[256] = 0; /* -.- */
	sprintf(strDur, "%f", (1.0/((double)current.sampleRate)));
	sprintf(strChan, "%d", current.channelCount);
	sprintf(strHeaderBytes, "%d", 256*(current.channelCount+1));
	sprintf(hbuf,
			"%- 8.8s" // data format version number
			"%- 80.80s"   // patient
			"%- 80.80s"   // recorder
			"%- 8.8s"     // date dd.mm.yy
			"%- 8.8s"     // time hh.mm.ss
			"%- 8.8s"     // bytes in header
			"%- 44.44s"   // reserved "EDF+C" indicator
			"%- 8.8s"     // number of data records or -1 for unknown for us
      "%- 8.8s"     // duration, in seconds
			"%- 4.4s",     // number of signals
			"0",
			getPatientName(),
			getRecorderName(),
			getDMY(),
			getHMS(),
			strHeaderBytes,
			"EDF+C",
			"-1",
			strDur,
			strChan
			);
#endif
	return hbuf;
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
			clients[clientCount].in.inbufCount = 0;
			clientCount += 1;
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
	close(serfd);
	return 0;
}

void handleSamples(unsigned int channelCount, const unsigned int *values)
{
//	printf("Got a %d-channel sample set: %08x, %08x\n", channelCount, values[0], values[1]);
	int i;
	char sbuf[4096];
	char *ptr;
	int len;
	ptr = sbuf;
	for (i = 0; i < channelCount; ++i)
		ptr += sprintf(ptr, "%d ", values[i]);
	len = strlen(sbuf);
	sbuf[len-1] = '\n';
	for (i = 0; i < clientCount; ++i) {
		if (!clients[i].isReceiving)
			continue;
		write(clients[i].fd, sbuf, len);
	}
}
