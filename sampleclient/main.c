#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <gtk/gtk.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <netdb.h>
#include <assert.h>
#include <sys/socket.h>

#define DEFAULTHOST "localhost"
#define DEFAULTPORT 8336
#define UPDATEINTERVAL 32


GtkWidget *window;
GtkWidget *onscreen;
GdkPixmap *offscreen = NULL;
unsigned char EDFHeader[256+32*256+1];
unsigned char responseStr[256];
int sock_fd;
int responseCode = 0;
int responsePos = 0;
int readPos = 0;
int haveReadEDFHeader = 0;
int awaitingResponse = 0;
int awaitingBanner = 0;


#define VIEWWIDTH 768
#define VIEWHEIGHT 128
#define TOPMARGIN 80
#define MARGIN 160

#define DRAWINGAREAHEIGHT (TOPMARGIN + VIEWHEIGHT*2 + MARGIN)

void sendCommand(int fd, const char *str)
{
	char buf[1024];
	int retval;
	sprintf(buf, "%s\n", str);
	awaitingResponse = 1;
	responsePos = 0;
	retval = write(fd, buf, strlen(buf));
	if (retval != strlen(buf)) {
		printf("Write error\n");
		exit(1);
	}
}

void getEDFHeader(int sock_fd)
{
	sleep(1);
	sendCommand(sock_fd, "GetEDFHeader");
}

int connectToNeuroServer()
{
	struct hostent *addr;
	struct sockaddr_in saddr;
	int sock_fd;
	int retval;
	addr = gethostbyname(DEFAULTHOST);
	if (addr == NULL) { // couldn't find host
		perror("gethostbyname");
		exit(1);
	}
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd < 0) {
		perror("socket");
		exit(1);
	}
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(DEFAULTPORT);
	saddr.sin_addr = *(struct in_addr *) addr->h_addr;
	retval = connect(sock_fd, (struct sockaddr *) &saddr, sizeof(saddr));
	if (retval != 0) {
		perror("connect");
		exit(1);
	}
	return sock_fd;
}

void handleResponseChar(char c)
{
	responseStr[responsePos++] = c;
	if (c == '\n') {
		char buf[4];
		responseStr[responsePos++] = 0;
		buf[0] = responseStr[0];
		buf[1] = responseStr[1];
		buf[2] = responseStr[2];
		buf[3] = 0;
		responseCode = atoi(buf);
		awaitingResponse = 0;
		strtok(responseStr, "\r\n"); // remove these
		printf("Got response <%s> and response code %d\n", responseStr, responseCode);
	}
}

void handleEDFHeaderChar(char c)
{
	static int bytesNeeded;
	EDFHeader[readPos++] = c;
	if (readPos == 256) {
		char buf[9];
		memcpy(buf, EDFHeader+184, 8);
		buf[8] = 0;
		bytesNeeded = atoi(buf);
	}
	if (readPos == bytesNeeded) {
		EDFHeader[readPos++] = '\0';
		printf("Here is the EDF Header:\n%s\n", EDFHeader);
		haveReadEDFHeader = 1;
		sendCommand(sock_fd, "SendSamples");
	}
}

void handleBannerChar(char c)
{
	static char nlCounter = 0;
	putchar(c);
	if (c == '\n') {
		nlCounter += 1;
		if (nlCounter == 1) {
			printf("Finished receiving banner.\n");
			awaitingBanner = 0;
		}
	}
}

static int sampleBuf[2][VIEWWIDTH];
static int readSamples = 0;

void handleSample(int channel, int val)
{
	static int updateCounter = 0;
	assert(channel == 0 || channel == 1);
	if (!(val >= 0 && val < 1024)) {
		printf("Got bad value: %d\n", val);
		exit(0);
	}
	if (readSamples == VIEWWIDTH-1)
		memmove(&sampleBuf[channel][0], &sampleBuf[channel][1], 
				     sizeof(int)*(VIEWWIDTH-1));
	sampleBuf[channel][readSamples] = val;
	if (readSamples < VIEWWIDTH-1 && channel == 1)
		readSamples += 1;
	if (updateCounter++ % UPDATEINTERVAL == 0) {
		gtk_widget_draw(onscreen, NULL);
	}
}

static gboolean destroy_event( GtkWidget *widget, GdkEventExpose *event )
{
	exit(0);
}

static gboolean expose_event( GtkWidget *widget, GdkEventExpose *event )
{
	int c, i;
	if (offscreen == NULL) {
		offscreen = gdk_pixmap_new(widget->window, VIEWWIDTH, DRAWINGAREAHEIGHT, -1);
	}
	gdk_draw_rectangle (offscreen,
		onscreen->style->white_gc,
		TRUE,
		0, 0,
		onscreen->allocation.width,
		onscreen->allocation.height);
		for (c = 0; c < 2; ++c) {
				for (i = 1; i < VIEWWIDTH; ++i) {
					gdk_draw_line(offscreen, onscreen->style->black_gc,
                        i-1, 
									TOPMARGIN + c*(VIEWHEIGHT+MARGIN) + (sampleBuf[c][i-1] * VIEWHEIGHT) / 1024,
                        i, 
									TOPMARGIN + c*(VIEWHEIGHT+MARGIN) + (sampleBuf[c][i] * VIEWHEIGHT) / 1024);
				}
			}
  gdk_draw_pixmap(widget->window,
		  widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		  offscreen,
		  event->area.x, event->area.y,
		  event->area.x, event->area.y,
		  event->area.width, event->area.height);

  return FALSE;
}

void handleSampleData(char c)
{
	static int curSample = 0;
	static int channelIndicator = 0;
	if (isdigit(c)) {
		curSample = curSample * 10 + (c - '0');
		return;
	}
	if (c == ' ') {
		handleSample(channelIndicator, curSample);
		curSample = 0;
		channelIndicator += 1;
		return;
	}
	if (c == '\r') return;
	if (c == '\n') {
		handleSample(channelIndicator, curSample);
		channelIndicator = 0;
		curSample = 0;
		return;
	}
	printf("Got bad char: %c\n", c);
}

gboolean readHandler(GIOChannel *source, GIOCondition cond, gpointer data)
{
	gchar c;
	gsize bytesRead = 0;
	GIOStatus retval;
	retval = g_io_channel_read_chars(source, &c, (gsize) 1, &bytesRead, NULL);
//	printf("retval is %d\n", retval);
//	printf("bytesRead is %d\n", bytesRead);
	if (bytesRead == 1) {
		if (awaitingBanner) {
			handleBannerChar(c);
			goto done;
		}
		if (awaitingResponse) {
			handleResponseChar(c);
			goto done;
		}
		if (!haveReadEDFHeader) {
			handleEDFHeaderChar(c);
			goto done;
		}
			handleSampleData(c);
	} else {
		printf("Server died / error.\n");
		exit(1);
	}
done:
	return TRUE;
}

int main( int   argc,
          char *argv[] )
{
	
	GIOChannel *neuroserver;
	
	gtk_init (&argc, &argv);
    
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	onscreen = (GtkWidget *) gtk_drawing_area_new();
	gtk_drawing_area_size((GtkDrawingArea *) onscreen, 
			                   VIEWWIDTH, DRAWINGAREAHEIGHT);
	gtk_container_add(GTK_CONTAINER (window), onscreen);
	g_signal_connect(G_OBJECT(onscreen), "expose_event", G_CALLBACK(expose_event),NULL );
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(destroy_event), NULL);
	gtk_widget_show(onscreen);
	gtk_widget_show(window);
    
	sock_fd = connectToNeuroServer();

	awaitingBanner = 1;

	neuroserver = g_io_channel_unix_new(sock_fd);

	g_io_add_watch(neuroserver, G_IO_IN, readHandler, NULL);

	getEDFHeader(sock_fd);

	gtk_main ();

	return 0;
}

