#ifndef __NS2NET_H
#define __NS2NET_H

struct NSNet;
struct NSNetConnectionController;

typedef void (*NSNetFunc)(void *udata);
typedef void (*NSNetReadFunc)(void *udata, const char *bytes, int len);
typedef void (*NSNetSuccessFunc)(void *udata, struct NSNetConnectionController *nscc);

struct NSNetConnectionHandler {
  NSNetSuccessFunc success;
  NSNetFunc refused;
  NSNetFunc timedOut;
  NSNetFunc unknownHost;
};

struct NSNetBindHandler {
  NSNetSuccessFunc success;
  NSNetFunc error;
};

struct NSNetConnectionControlHandler {
  NSNetFunc closed;
  NSNetReadFunc bytesRead;
};

struct NSNet *newNSNet(void);
void waitForNetEvent(struct NSNet *ns, int timeout);
void closeNSNet(struct NSNet *ns);
int attemptBind(struct NSNet *ns, const struct NSNetBindHandler *nsb,
                int isLocalOnly, unsigned short portNum, void *udata);
int attemptConnect(struct NSNet *ns, const struct NSNetConnectionHandler *nsc,
                    const char *destaddr, unsigned short destPort, void *udata);

int attachConnectionControlHandler(struct NSNetConnectionController *nscc,
    const struct NSNetConnectionControlHandler *nscch);
int writeNSBytes(struct NSNetConnectionController *nscc, void *buf, int len);
int closeConnection(struct NSNetConnectionController *nscc);

#endif

