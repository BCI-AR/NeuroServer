#ifndef __NSREALLOC_H
#define __NSREALLOC_H

struct NSByteHolder;

struct NSByteHolder *newNSByteHolder(void);
void addCharacter(struct NSByteHolder *nsb, char c);
int getSize(struct NSByteHolder *nsb);
const char *getData(struct NSByteHolder *nsb);
void freeNSByteHolder(struct NSByteHolder *nsb);

#endif
