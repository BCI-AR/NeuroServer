#include <malloc.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <nsnet.h>
#include <fcntl.h>
#include <assert.h>
#include <neuro/neuro.h>
#include <neuro/ns2net.h>


struct NSNetConnectionController {
  struct NSNet *ns;
  sock_t fd;
};

struct NSNetConnectionHandlerInternal {
  sock_t fd;
  rsockaddr_t dest;
  struct NSNetConnectionHandler nsc;
  struct NSNetConnectionController nscc;
  void *udata;
};

#define MAXCONN 256

struct NSNet {
  sock_t max_fd;
  fd_set readfds, writefds, errfds;
  int NSCICount;
  struct NSNetConnectionHandlerInternal nsci[MAXCONN];
};

int setNSnonblocking(struct NSNet *ns, sock_t sock_fd)
{
  int flags, retval;	
  flags = fcntl(sock_fd, F_GETFL, 0);
  retval = fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK);
  assert(retval == 0 && "Error setting O_NONBLOCK\n");
  return 0;
}

static void updateNSNetMaxFd(struct NSNet *ns, sock_t fd)
{
  if (ns->max_fd < fd+1)
    ns->max_fd = fd+1;
}

/* Returns 0 on success 
 * \param name The hostname to connect to, or dotted quad
 * \param portNo The integer network port number to connect to
 * \param result The address of the rsockaddr_t that will receive the lookup
 * \param nsc The NSConnectionHandler that may be used for unknownHost messages
 * \param udata Only for use when an unknown host error occurs on callback
 *  This function returns 1 on unknown host failures.
 */
static int makeAddress(const char *name, unsigned short portNo, rsockaddr_t *result, const struct NSNetConnectionHandler *nsc, void *udata)
{
  hostent_t host;
  host = gethostbyname(name);
  if(host == NULL) {
    nsc->unknownHost(udata);
    return 1;
  }
  result->sin_family = AF_INET;
  result->sin_port = htons(portNo);
  result->sin_addr = *((struct in_addr *)host->h_addr);
  return 0;
}

static void addWriteFd(struct NSNet *ns, sock_t fd)
{
  FD_SET(fd, &ns->writefds);
  updateNSNetMaxFd(ns, fd);
}

static void addErrFd(struct NSNet *ns, sock_t fd)
{
  FD_SET(fd, &ns->errfds);
  updateNSNetMaxFd(ns, fd);
}

static void addReadFd(struct NSNet *ns, sock_t fd)
{
  FD_SET(fd, &ns->readfds);
  updateNSNetMaxFd(ns, fd);
}

static void removeFdAll(struct NSNet *ns, sock_t fd)
{
  FD_CLR(fd, &ns->readfds);
  FD_CLR(fd, &ns->writefds);
  FD_CLR(fd, &ns->errfds);
}

int attemptConnect(struct NSNet *ns, const struct NSNetConnectionHandler *nsc,
                    const char *destaddr, int destPort, void *udata)
{
  int retval;
  int num;
  int connErrno = 0;
  sock_t fd;
  rsockaddr_t dest;
  if (makeAddress(destaddr, destPort, &dest, nsc, udata)) {
    return 1;
  }
  fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  setNSnonblocking(ns, fd);
  errno = 0;
  retval = connect(fd, (struct sockaddr *) &dest, sizeof(dest));
  if (retval == -1) connErrno = errno;
  if (retval == 0) {
    struct NSNetConnectionController *nscc = calloc(sizeof(*nscc), 1);
    nscc->ns = ns;
    nscc->fd = fd;
    nsc->success(udata, nscc);
    return 0;
  }
  rassert(retval == -1);
  if (connErrno == EINPROGRESS) {
    num = ns->NSCICount;
    ns->NSCICount += 1;
    ns->nsci[num].nsc = *nsc;
    ns->nsci[num].fd = fd;
    ns->nsci[num].dest = dest;
    ns->nsci[num].udata = udata;
    addWriteFd(ns, fd);
    addErrFd(ns, fd);
    addReadFd(ns, fd);
    return 1;
  }
  rassert(0 && "Unkown connect error");
  return 1;
}

#if 0
static char *showFD(sock_t fd, fd_set *readfds, fd_set *writefds, fd_set *errfds) {
  static char buf[16];
  buf[0] = 0;
  if (FD_ISSET(fd, readfds)) strcat(buf, "RE ");
  if (FD_ISSET(fd, writefds)) strcat(buf, "WR ");
  if (FD_ISSET(fd, errfds)) strcat(buf, "EX ");
  if (buf[0] != 0)
    buf[strlen(buf)-1] = 0;
  return buf;
}
#endif

void waitForNetEvent(struct NSNet *ns, int timeout)
{
  fd_set readfds, writefds, errfds;
  sock_t max_fd;
  struct timeval tv;
  int retval;
  int i;
  readfds = ns->readfds;
  writefds = ns->writefds;
  errfds = ns->errfds;
  max_fd = ns->max_fd;
  tv.tv_sec = timeout / 1000;
  tv.tv_usec = (timeout % 1000) * 1000;
  retval = select(max_fd, &readfds, &writefds, &errfds, &tv);
  if (retval > 0) 
  { // There are some fds ready
    int connErrno, connErrnoLen = sizeof(connErrno);
    for (i = 0; i < ns->NSCICount; i+=1)
    {
      if (FD_ISSET(ns->nsci[i].fd, &writefds) || FD_ISSET(ns->nsci[i].fd, &errfds) || FD_ISSET(ns->nsci[i].fd, &readfds)) 
      {
        if (getsockopt(ns->nsci[i].fd, SOL_SOCKET, SO_ERROR, &connErrno, &connErrnoLen) == 0) 
        {
        struct NSNetConnectionController *nscc;
          switch(connErrno)
          {
            case ECONNREFUSED:
              ns->nsci[i].nsc.refused(ns->nsci[i].udata);
              removeFdAll(ns, ns->nsci[i].fd);
              break;
            case 0:
              nscc = calloc(sizeof(*nscc), 1);
              nscc->ns = ns;
              nscc->fd = ns->nsci[i].fd;
              ns->nsci[i].nsc.success(ns->nsci[i].udata, nscc);
              removeFdAll(ns, ns->nsci[i].fd);
              break;
            default:
              printf("Unhandled connErrno: %d\n", connErrno);
              exit(1);
              break;
          }
          return;
        }
      }
    }
  }
  else {
    for (i = 0; i < ns->NSCICount; ++i) {
      ns->nsci[i].nsc.timedOut(ns->nsci[i].udata);
    }
  }
}

struct NSNet *newNSNet(void)
{
  struct NSNet *result;
  result = calloc(sizeof(struct NSNet), 1);
  FD_ZERO(&result->readfds);
  FD_ZERO(&result->writefds);
  FD_ZERO(&result->errfds);
  return result;
}

void closeNSNet(struct NSNet *ns)
{
  free(ns);
}


