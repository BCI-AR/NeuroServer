#ifndef __NS2NET_H
#define __NS2NET_H

struct NSNet;
typedef void (*NSNetFunc)(void *udata);

struct NSNetConnectionHandler {
  NSNetFunc success;
  NSNetFunc refused;
  NSNetFunc timedOut;
  NSNetFunc unknownHost;
};

struct NSNet *newNSNet(void);
void waitForNetEvent(struct NSNet *ns, int timeout);
void closeNSNet(struct NSNet *ns);
int attemptConnect(struct NSNet *ns, const struct NSNetConnectionHandler *nsc,
                    const char *destaddr, int destPort, void *udata);

#endif

