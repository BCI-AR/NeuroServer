#ifndef __NEURO_READERS_H
#define __NEURO_READERS_H


struct NSBlockReader;
struct NSLineReader;
struct NSCompositeReader;

struct NSBlockHandler {
  void (*handleBlock)(const char *block, int blockLen, const void *udata);
};

struct NSLineHandler {
  void (*handleLine)(const char *line, int lineLen, const void *data);
};

struct NSCompositeHandler {
  void (*handleBlock)(const char *block, int blockLen, const void *udata);
  void (*handleLine)(const char *line, int lineLen, const void *data);
};

struct NSBlockReader *newNSBlockReader(int howManyBytes, const struct NSBlockHandler *block, void *udata);
struct NSLineReader *newNSLineReader(const struct NSLineHandler *line, void *udata);
struct NSCompositeReader *newNSCompositeReader(const struct NSCompositeHandler *nsc, void *udata);

void readNewBlock(struct NSCompositeReader *nsc, int howManyBytes);

void freeNSBlockReader(struct NSBlockReader *nsb);
void freeNSLineReader(struct NSLineReader *nsl);
void freeNSCompositeReader(struct NSCompositeReader *nsc);

#endif

