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
                

#ifndef __NSNET_H
#define __NSNET_H
#include <nsutil.h>

#define CHUNKSIZE 256

#define DEFAULTPORT 8336
#define DEFAULTHOST "localhost"
#define MAXCLIENTS 16

#ifdef __MINGW32__
#include <winsock2.h>
#define sock_t SOCKET
#include <sys/types.h>
#define rsockaddr_t SOCKADDR_IN
#define hostent_t LPHOSTENT
#else
#include <sys/types.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
typedef struct hostent *hostent_t;
#define sock_t int
#define rsockaddr_t struct sockaddr_in
#endif

struct InputBuffer {
	int read_cnt;
	int isEOF;
	char *read_ptr;
	char read_buf[MAXLEN];
};

struct OutputBuffer {
	int write_cnt;
	char write_buf[MAXLEN];
};

sock_t rsocket(void);
int readline(sock_t fd, char *vptr, size_t maxlen, struct InputBuffer *ib);
const char *stringifyErrorCode(int code);
int rbindAll(sock_t sock_fd);
int writeBytes(sock_t con, const char *buf, int size, struct OutputBuffer *ob);
int writeString(sock_t con, const char *buf, struct OutputBuffer *ob);
int rconnectName(sock_t sock_fd, const char *hostname, unsigned short portno);
int rinitNetworking(void);
sock_t raccept(sock_t sock_fd);
void setblocking(sock_t sock_fd);
void setnonblocking(sock_t sock_fd);
void rshutdown(sock_t fd);
/// Returns 1 if EOF 
int getResponseCode(sock_t sock_fd, struct InputBuffer *);
int getOK(sock_t sock_fd, struct InputBuffer *);
int rrecv(sock_t fd, char *read_buf, size_t count);
void initInputBuffer(struct InputBuffer *ib);
void initOutputBuffer(struct OutputBuffer *ob);
int isEOF(sock_t con, const struct InputBuffer *ib);
int inputBufferEmpty(const struct InputBuffer *ib);
int my_read(sock_t fd, char *ptr, size_t maxlen, struct InputBuffer *);
size_t readlinebuf(void **vptrptr, struct InputBuffer *);
size_t writen(sock_t fd, const void *vptr, size_t len, struct OutputBuffer *ob);
size_t readn(sock_t fd, void *vptr, size_t len, struct InputBuffer *ib);
int rselect(sock_t max_fd, fd_set *read, fd_set *write, fd_set *err);
int rselect_timed(sock_t max_fd, fd_set *toread, fd_set *towrite, 
	fd_set *toerr, struct timeval *tv);
/// Searches for a given socket \param fd in the handler table, \a fdTable, returning index or -1 if not found
int findFdIndex(sock_t fd);
#endif
