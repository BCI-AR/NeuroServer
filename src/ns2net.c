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
  void *udata;
  rsockaddr_t peer;
  sock_t fd;
};

struct NSNetReaderHandlerInternal {
  struct NSNet *ns;
  sock_t fd;
  void *udata;
  struct NSNetConnectionReadHandler nsrh;
};

struct NSNetBindHandlerInternal {
  struct NSNet *ns;
  sock_t fd;
  void *udata;
  struct NSNetBindHandler nsb;
};

struct NSNetConnectionHandlerInternal {
  struct NSNet *ns;
  sock_t fd;
  void *udata;
  struct NSNetConnectionHandler nsc;
  rsockaddr_t dest;
  struct NSNetConnectionController nscc;
};

#define MAXCONN 256

struct NSNet {
  sock_t max_fd;
  fd_set readfds, writefds, errfds;
  int NSCICount;
  struct StringTable *connectMap;
  struct StringTable *bindMap;
  struct StringTable *readerMap;
};

static void addWriteFd(struct NSNet *ns, sock_t fd);
static void addReadFd(struct NSNet *ns, sock_t fd);
static void addErrFd(struct NSNet *ns, sock_t fd);

int attachConnectionReadHandler(struct NSNetConnectionController *nscc,
    const struct NSNetConnectionReadHandler *nscch)
{
  struct NSNet *ns;
  ns = nscc->ns;
  //putInt(ns->readerMap, nscc->fd, nscch);
  return 0;
}

int attemptBind(struct NSNet *ns, const struct NSNetBindHandler *nsb,
                int isLocalOnly, unsigned short portNum, void *udata)
{
  struct NSNetBindHandlerInternal *bindHI;

  sock_t sock_fd;
  rsockaddr_t local;
  int retval;
  int one = 1;

  sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock_fd < 0) {
    perror("socket");
    nsb->error(udata);
    return 1;
  }
  retval = setsockopt(sock_fd,SOL_SOCKET,SO_REUSEADDR,(char *)&one,sizeof(one));

  if (retval != 0) {
    perror("setsockopt");
    nsb->error(udata);
    return 1;
  }

  local.sin_family = AF_INET;
  local.sin_port = htons(portNum);
  if (isLocalOnly) {
    retval = inet_aton("127.0.0.1", &local.sin_addr);
    if (retval == 0) {
      perror("inet_aton");
      nsb->error(udata);
      return 1;
    }
  }
  else
    local.sin_addr.s_addr =  INADDR_ANY;

  retval = bind(sock_fd, (struct sockaddr *) &local, sizeof(local));
  if (retval != 0) {
    perror("bind");
    nsb->error(udata);
    return 1;
  }
  retval = listen(sock_fd, MAXCLIENTS);
  if (retval != 0) {
    perror("listen");
    nsb->error(udata);
    return 1;
  }

  addErrFd(ns, sock_fd);
  addReadFd(ns, sock_fd);

  bindHI = calloc(sizeof(*bindHI), 1);
  bindHI->fd = sock_fd;
  bindHI->udata = udata;
  bindHI->nsb = *nsb;

  putInt(ns->bindMap, sock_fd, bindHI);

  return 0;
}

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
                    const char *destaddr, unsigned short destPort, void *udata)
{
  int retval;
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
    nscc->peer = dest;
    nscc->udata = udata;
    nsc->success(udata, nscc);
    return 0;
  }
  rassert(retval == -1);
  if (connErrno == EINPROGRESS) {
    struct NSNetConnectionHandlerInternal *connectHI;
    connectHI = calloc(sizeof(*connectHI), 1);
    connectHI->ns = ns;
    connectHI->nsc = *nsc;
    connectHI->fd = fd;
    connectHI->dest = dest;
    connectHI->udata = udata;
    addWriteFd(ns, fd);
    addErrFd(ns, fd);
    addReadFd(ns, fd);
    putInt(ns->connectMap, fd, connectHI);
    return 0;
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

static void removeConnectionHandlerInternal(struct NSNet *ns, struct NSNetConnectionHandlerInternal *connectHI)
{
  delInt(ns->connectMap, connectHI->fd);
  free(connectHI);
}

void waitForNetEvent(struct NSNet *ns, int timeout)
{
  fd_set readfds, writefds, errfds;
  sock_t max_fd;
  struct timeval tv;
  struct NSNetConnectionController *nscc;
  struct NSNetBindHandlerInternal *bindHI;
  struct NSNetConnectionHandlerInternal *connectHI;
  //struct NSNetReaderHandlerInternal *readerHI;
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
    for (i = 0; i < ns->max_fd; i+=1)
    {
      if (FD_ISSET(i, &readfds) && (bindHI = findInt(ns->bindMap, i))) {
        int addrlen = sizeof(nscc->peer);
        nscc = calloc(sizeof(*nscc), 1);
        nscc->ns = ns;
        nscc->udata = bindHI->udata;
        nscc->fd = accept(i, (struct sockaddr *) &nscc->peer, &addrlen);
        bindHI->nsb.success(bindHI->udata, nscc);
      }
    }
    for (i = 0; i < ns->max_fd; i+= 1)
    {
      if ((FD_ISSET(i, &writefds) || FD_ISSET(i, &errfds) || FD_ISSET(i, &readfds)) && (connectHI = findInt(ns->connectMap, i)))
      {
        if (getsockopt(connectHI->fd, SOL_SOCKET, SO_ERROR, &connErrno, &connErrnoLen) == 0) 
        {
          switch(connErrno)
          {
            case ECONNREFUSED:
              connectHI->nsc.refused(connectHI->udata);
              removeFdAll(ns, connectHI->fd);
              removeConnectionHandlerInternal(ns, connectHI);
              break;
            case 0:
              nscc = calloc(sizeof(*nscc), 1);
              nscc->ns = ns;
              nscc->peer = connectHI->dest;
              nscc->fd = connectHI->fd;
              nscc->udata = connectHI->udata;
              connectHI->nsc.success(connectHI->udata, nscc);
              removeFdAll(ns, connectHI->fd);
              removeConnectionHandlerInternal(ns, connectHI);
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
}

struct NSNet *newNSNet(void)
{
  struct NSNet *result;
  result = calloc(sizeof(struct NSNet), 1);
  FD_ZERO(&result->readfds);
  FD_ZERO(&result->writefds);
  FD_ZERO(&result->errfds);
  result->bindMap = newStringTable();
  result->connectMap = newStringTable();
  result->readerMap = newStringTable();
  return result;
}

void closeNSNet(struct NSNet *ns)
{
  freeStringTable(ns->bindMap);
  freeStringTable(ns->connectMap);
  freeStringTable(ns->readerMap);
  free(ns);
}


