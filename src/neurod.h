#ifndef __NEUROD_H
#define __NEUROD_H 1

#define MAX_CLIENTS 64
#define DEFAULT_PORT 8336
#define BANNERSTR "Welcome to NeuroServer v 0.31\r\n"

#define MAXLINELENGTH 80

struct InputBuffer;

void handleSamples(unsigned int channelCount, const unsigned int *values);
void watchForP2(unsigned char nextByte);
void watchForP3(unsigned char nextByte);
const char *printEDFHeader(void);
const char *printChannelBlock(void);

void eatCharacter(struct InputBuffer *ib, unsigned char nextChar);

#endif

