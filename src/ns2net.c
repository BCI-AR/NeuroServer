#include <malloc.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <nsnet.h>
#include <fcntl.h>
#include <assert.h>
#include <neuro/ns2net.h>


struct NSNetConnectionHandlerInternal {
  sock_t fd;
  rsockaddr_t dest;
  struct NSNetConnectionHandler nsc;
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
  sock_t fd;
  rsockaddr_t dest;
  if (makeAddress(destaddr, destPort, &dest, nsc, udata)) {
    return 1;
  }
  fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  printf("Got socket!\n");
  setNSnonblocking(ns, fd);
  printf("set nonblock socket!\n");
  retval = connect(fd, (struct sockaddr *) &dest, sizeof(dest));
  printf("got %d from connect\n", retval);
  if (retval == 0) {
    nsc->success(udata);
    return 0;
  }
  assert(retval == -1);
  perror("Connect");
  if (errno == EINPROGRESS) {
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
  return 1;
}

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

void waitForNetEvent(struct NSNet *ns, int timeout)
{
  fd_set readfds, writefds, errfds;
  sock_t max_fd;
  struct timeval tv;
  int retval;
  readfds = ns->readfds;
  writefds = ns->writefds;
  errfds = ns->errfds;
  max_fd = ns->max_fd;
  tv.tv_sec = timeout / 1000;
  tv.tv_usec = (timeout % 1000) * 1000;
  printf("selecting...\n");
  retval = select(max_fd, &readfds, &writefds, &errfds, &tv);
  printf("Got %d descriptors\n", retval);
  if (retval > 0) { // There are some fds ready
    int i;
    for (i = 0; i < ns->NSCICount; i+=1) {
      if (FD_ISSET(ns->nsci[i].fd, &writefds) || FD_ISSET(ns->nsci[i].fd, &errfds) || FD_ISSET(ns->nsci[i].fd, &readfds)) {
        removeFdAll(ns, ns->nsci[i].fd);
        printf("Inside WaitFor with %s\n", showFD(ns->nsci[i].fd, &readfds, &writefds, &errfds));
//        retval = connect(ns->nsci[i].fd, (struct sockaddr *) &(ns->nsci[i].dest), sizeof(struct sockaddr));
        if (FD_ISSET(ns->nsci[i].fd, &errfds)) {
          ns->nsci[i].nsc.refused(ns->nsci[i].udata);
        } else {
          ns->nsci[i].nsc.success(ns->nsci[i].udata);
        }
      }
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


