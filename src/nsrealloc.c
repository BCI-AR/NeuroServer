#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <neuro/nsrealloc.h>

#define STARTINGBUFSIZE 256

struct NSByteHolder
{
  int alloc, used;
  char *buf;
};

static void ensureMoreSpace(struct NSByteHolder *nsb)
{
  if (nsb->used == nsb->alloc)
    {
    int newSize;
    void *newbuf;
    newSize = nsb->alloc * 2;
    newbuf = calloc(1, newSize);
    memcpy(newbuf, nsb->buf, nsb->used);
    free(nsb->buf);
    nsb->buf = newbuf;
    nsb->alloc = newSize;
    }
}

struct NSByteHolder *newNSByteHolder(void)
{
  struct NSByteHolder *nsb;
  nsb = calloc(1, sizeof(struct NSByteHolder));
  nsb->buf = calloc(1, STARTINGBUFSIZE);
  nsb->alloc = STARTINGBUFSIZE;
  nsb->used = 0;
  return nsb;
}

void addCharacter(struct NSByteHolder *nsb, char c)
{
  ensureMoreSpace(nsb);
  nsb->buf[nsb->used] = c;
  nsb->used += 1;
}

int getSize(struct NSByteHolder *nsb)
{
  return nsb->used;
}

const char *getData(struct NSByteHolder *nsb)
{
  return nsb->buf;
}

void freeNSByteHolder(struct NSByteHolder *nsb)
{
  nsb->alloc = 0;
  nsb->used = 0;
  free(nsb->buf);
  nsb->buf = NULL;
  free(nsb);
}

