#ifndef __NS2NET_H
#define __NS2NET_H

/**
  *
  * \file ns2net.h
  * \author Rudi Cilibrasi
  *
  */

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

struct NSNetConnectionReadHandler {
  NSNetFunc closed;
  NSNetReadFunc bytesRead;
};

/**
 * 
 * \brief Initializes networking in a system-independent way.
 * \return A pointer to an opaque struct NSNet
 */
struct NSNet *newNSNet(void);
/**
 * \brief Waits for a network event to arrive, and dispatches it, or times out.
 * \param timeout a timeout value in milliseconds.  The function will return if no event occurs when the timeout value has elapsed.
 */
void waitForNetEvent(struct NSNet *ns, int timeout);

/**
 * \brief Closes an NSNet interface, freeing the memory and other resources associated with it.
 * \param ns A pointer to the NSNet structure that was opened previously using newNSNet()
 */
void closeNSNet(struct NSNet *ns);
/**
 * \brief Performs an asynchronous bind on a network port and begins listening for connections.
 * \param ns Pointer to a struct NSNet
 * \param nsb Pointer to a user-supplied struct NSNetBindHandler function table
 * \param isLocalOnly a boolean flag indicating whether only the local loopback interface should be bound if nonzero, or all network interfaces if 0.
 * \param portNum a network port number to bind a listening socket
 * \param udata User-defined data to be passed to the handler functions
 *
 * This function opens a new network socket and binds it to a listening
 * port.  New connections or errors are communicated via callback functions in
 * nsb.  If clients attempt to connect to this port, then the nsb success()
 * function will be called.  If there is a network error,
 * then nsb error() function will be called.  Note that success may
 * be called multiple times if there are multiple connections.  Each
 * time success() is called, the user-data supplied as udata will be passed
 * as well as a new struct NSNetConnectionController to handle reading
 * and writing on the newly accepted connection.
 * Note that since this function is asynchronous, it will return immediately,
 * and if your application wants to wait for an incoming connection, it must
 * use waitForNetEvent() after calling this one.
 */
int attemptBind(struct NSNet *ns, const struct NSNetBindHandler *nsb,
                int isLocalOnly, unsigned short portNum, void *udata);
/**
 * \brief Creates a new socket and initiates an asychronous connection request to the specified hostname and port.
 * \param ns Pointer to a struct NSNet
 * \param nsb User-defined callback functions for later results
 * \param destaddr A string containing either a dotted-quad IP address or a hostname specifying the destination address to connect to
 * \param destPort The network port number to connect to
 * \param udata User-supplied data to be passed on to the handler functions
 * \return 0 upon successful start of the connection request.
 *
 * Creates a new network socket and begins trying to connect to the address
 * specified by destaddr and destPort.  Since this function is asynchronous, it
 * returns immediately.  If the connection is successful, the nsb success()
 * function will be called.  If the connection is refused, for instance
 * because the destination machine is not listening on the specified port,
 * then the nsb refused() function will be called.  If a hostname is supplied
 * and there is no address available via DNS lookup, then unknownHost will be
 * called.  If too much time passes trying to connect, then timedOut will be
 * called.
 */
int attemptConnect(struct NSNet *ns, const struct NSNetConnectionHandler *nsc,
                    const char *destaddr, unsigned short destPort, void *udata);
/**
 * \brief Attaches a struct NSNetConnectionReadHandler to a live network connetion.
 * \param nscc a pointer to the struct NSNetConnectionController in charge of the network connection
 * \param nscch a pointer to a callback function block in a struct NSNetConnectionReadHandler
 * \param udata a pointer to user-defined data to be passed to the handler functions
 * \return 0 on success
 *
 * Once a connection is established, from a successful attemptConnect() or
 * attemptBind(), a read handler may be attached to the associated struct
 * NSNetConnectionController.  Once a read handler is attached, it will
 * receive to types of callback messages:
 * readBytes will be called when new bytes are read from the connection.
 * closed will be called if either side closes the connection, or if
 * the connection closes unexpectedly due to a network-level error.
 */
int attachConnectionReadHandler(struct NSNetConnectionController *nscc,
    const struct NSNetConnectionReadHandler *nscch, void *udata);
/**
 * \brief Writes a number of bytes to a network connection.
 * \param nscc The struct NSNetConnectionController associated with the network connection.
 * \param buf The memory buffer holding the bytes to be written
 * \param len The number of bytes to be written
 * \return 0 on success, or nonzero if a writing error occurs before all bytes
 * have been written.
 * Before using this function, you must obtain a struct
 * NSNetConnectionController via callback functions (success) from
 * attemptBind() or attemptConnection()
 */
int writeNSBytes(struct NSNetConnectionController *nscc, void *buf, int len);

/**
 * \brief Closes a network connection.
 * \param The struct NSNetConnectionController associated with the network connection to be closed.
 * \return 0 on success.
 * This function closes a network connection.  Note that if there are readers
 * attached on either or both sides, their close() method will be called.
 */
int closeConnection(struct NSNetConnectionController *nscc);

#endif

