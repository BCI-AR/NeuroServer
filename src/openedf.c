#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <openedf.h>

#define DECODEHEADERFIELD(packet, fieldName) \
				packet+offsetof(struct EDFPackedHeader,fieldName), sizeof((struct EDFPackedHeader *)0)->fieldName

#define DECODECHANNELFIELD(packet, fieldName, whichChannel, totalChannels) \
				packet+totalChannels*offsetof(struct EDFPackedChannelHeader,fieldName) + whichChannel*sizeof(((struct EDFPackedChannelHeader *)0)->fieldName),sizeof((struct EDFPackedChannelHeader *)0)->fieldName

#define LOADHFI(m) \
	result->m = EDFUnpackInt(DECODEHEADERFIELD(packet, m))
#define LOADHFD(m) \
	result->m = EDFUnpackDouble(DECODEHEADERFIELD(packet, m))
#define LOADHFS(m) \
	strcpy(result->m, EDFUnpackString(DECODEHEADERFIELD(packet, m)))

#define LOADCFI(m) \
	result->m = EDFUnpackInt(DECODECHANNELFIELD(packet, m, whichChannel, totalChannels))
#define LOADCFD(m) \
	result->m = EDFUnpackDouble(DECODECHANNELFIELD(packet, m, whichChannel, totalChannels))
#define LOADCFS(m) \
	strcpy(result->m, EDFUnpackString(DECODECHANNELFIELD(packet, m, whichChannel, totalChannels)))

#define STORECFI(m, i) \
	storeEDFInt(DECODECHANNELFIELD(packet, m, whichChannel, totalChannels), i)
#define STORECFD(m, d) \
	storeEDFDouble(DECODECHANNELFIELD(packet, m, whichChannel, totalChannels), d)
#define STORECFS(m, s) \
	storeEDFString(DECODECHANNELFIELD(packet, m, whichChannel, totalChannels), s)

#define STOREHFI(m, i) \
	storeEDFInt(DECODEHEADERFIELD(packet, m), i)
#define STOREHFD(m, d) \
	storeEDFDouble(DECODEHEADERFIELD(packet, m), d)
#define STOREHFS(m, s) \
	storeEDFString(DECODEHEADERFIELD(packet, m), s)

void printChannelHeader(const struct EDFDecodedChannelHeader *chdr);

int EDFUnpackInt(const char *inp, int fieldLen)
{
	return atoi(EDFUnpackString(inp, fieldLen));
}

double EDFUnpackDouble(const char *inp, int fieldLen)
{
	return atof(EDFUnpackString(inp, fieldLen));
}

const char *EDFUnpackString(const char *inp, int fieldLen)
{
	static char retbuf[256];
	strncpy(retbuf, inp, fieldLen);
	retbuf[fieldLen] = '\0';
	return retbuf;
}

void storeEDFString(char *packet, size_t memsize, const char *str)
{
	char fmt[8];
	char buf[257];
	sprintf(fmt, "%%- %d.%ds");
	sprintf(buf, fmt, str);
	memcpy(packet, buf, memsize);
}

void storeEDFDouble(char *packet, size_t memsize, double d)
{
	char buf[64];
	sprintf(buf, "%f", d);
	storeEDFString(packet, memsize, buf);
}

void storeEDFInt(char *packet, size_t memsize, int i)
{
	char buf[64];
	sprintf(buf, "%d", i);
	storeEDFString(packet, memsize, buf);
}

int EDFEncodePacket(char *packet, const struct EDFDecodedConfig *cfg)
{
	int whichChannel, totalChannels;
	STOREHFS(dataFormat, cfg->hdr.dataFormat);
	STOREHFS(localPatient, cfg->hdr.localPatient);
	STOREHFS(localRecorder, cfg->hdr.localRecorder);
	STOREHFS(recordingStartDate, cfg->hdr.recordingStartDate);
	STOREHFS(recordingStartTime, cfg->hdr.recordingStartTime);
	STOREHFI(headerRecordBytes, cfg->hdr.headerRecordBytes);
	STOREHFS(manufacturerID, cfg->hdr.manufacturerID);
	STOREHFI(dataRecordCount, cfg->hdr.dataRecordCount);
	STOREHFD(dataRecordSeconds, cfg->hdr.dataRecordSeconds);
	STOREHFI(dataRecordChannels, cfg->hdr.dataRecordChannels);
	totalChannels = cfg->hdr.dataRecordChannels;
	for (whichChannel = 0; whichChannel < totalChannels; whichChannel++) {
		STORECFS(label, cfg->chan[whichChannel].label);
		STORECFS(transducer, cfg->chan[whichChannel].transducer);
		STORECFS(dimUnit, cfg->chan[whichChannel].dimUnit);
		STORECFD(physMin, cfg->chan[whichChannel].physMin);
		STORECFD(physMax, cfg->chan[whichChannel].physMax);
		STORECFD(digiMin, cfg->chan[whichChannel].digiMin);
		STORECFD(digiMax, cfg->chan[whichChannel].digiMax);
		STORECFS(prefiltering, cfg->chan[whichChannel].prefiltering);
		STORECFI(sampleCount, cfg->chan[whichChannel].sampleCount);
		STORECFS(reserved, cfg->chan[whichChannel].reserved);
	}
}

void EDFDecodeChannelHeader(struct EDFDecodedChannelHeader *result, const char *packet, int whichChannel, int totalChannels)
{
	struct EDFPackedChannelHeader *p = (struct EDFPackedChannelHeader *) packet;
	const char *src, *dest;
	memset((char *) result, 0, sizeof(*result));
	LOADCFS(label);
	LOADCFS(transducer);
	LOADCFS(dimUnit);
	LOADCFD(physMin);
	LOADCFD(physMax);
	LOADCFD(digiMin);
	LOADCFD(digiMax);
	LOADCFS(prefiltering);
	LOADCFI(sampleCount);
	LOADCFS(reserved);
}

void EDFDecodeHeaderPreamble(struct EDFDecodedHeader *result, const char *packet) {
	struct EDFPackedHeader *p = (struct EDFPackedHeader *) packet;
	const char *src, *dest;
	memset((char *) result, 0, sizeof(*result));
	LOADHFS(dataFormat);
	LOADHFS(localPatient);
	LOADHFS(localRecorder);
	LOADHFS(recordingStartDate);
	LOADHFS(recordingStartTime);
	LOADHFI(headerRecordBytes);
	LOADHFS(manufacturerID);
	LOADHFI(dataRecordCount);
	LOADHFD(dataRecordSeconds);
	LOADHFI(dataRecordChannels);
}

// Must call setEDFHeaderBytes() before writeEDF
int writeEDF(int fd, const struct EDFDecodedConfig *cfg)
{
	int retval;
	char pktbuf[(MAXCHANNELS+1) * 256];
	retval = EDFEncodePacket(pktbuf, cfg);
	if (retval) return retval;
	retval = write(fd, pktbuf, cfg->hdr.headerRecordBytes);
	if (retval != cfg->hdr.headerRecordBytes) {
		perror("write");
		return 1;
	}
	return 0;
}

int setEDFHeaderBytes(struct EDFDecodedConfig *cfg)
{
	cfg->hdr.headerRecordBytes = 256 * (cfg->hdr.dataRecordChannels + 1);
	return 0;
}

int readEDF(int fd, struct EDFDecodedConfig *cfg)
{
	int i;
	int retval;
	char pktbuf[(MAXCHANNELS+1) * 256];
	retval = read(fd, pktbuf, 256);
	if (retval != 256) {
		perror("read");
		return 1;
	}
	EDFDecodeHeaderPreamble(&cfg->hdr, pktbuf);
	assert(cfg->hdr.dataRecordChannels > 0);
	assert(cfg->hdr.dataRecordChannels < MAXCHANNELS);
	setEDFHeaderBytes(cfg);
	retval = read(fd, pktbuf+256, cfg->hdr.headerRecordBytes - 256);
	if (retval != cfg->hdr.headerRecordBytes - 256) {
		perror("read");
		return 1;
	}
	for (i = 0; i < cfg->hdr.dataRecordChannels; ++i) {
		EDFDecodeChannelHeader(&cfg->chan[i], pktbuf+256, i, cfg->hdr.dataRecordChannels);
	}
	return 0;
}

void printChannelHeader(const struct EDFDecodedChannelHeader *chdr)
{
	printf("The number of samples is %d\n", chdr->sampleCount);
	printf("The channel name is %s\n", chdr->label);
}

double getSecondsPerSample(const struct EDFDecodedConfig *cfg, int whichChan)
{
	return (double) cfg->hdr.dataRecordSeconds / cfg->chan[whichChan].sampleCount;
}

double getSamplesPerSecond(const struct EDFDecodedConfig *cfg, int whichChan)
{
	return (double) cfg->chan[whichChan].sampleCount / cfg->hdr.dataRecordSeconds;
}

int getDataRecordChunkSize(const struct EDFDecodedConfig *cfg)
{
	int i, sum=0;
	for (i = 0; i < cfg->hdr.dataRecordChannels; ++i)
		sum += cfg->chan[i].sampleCount;
	return sum * BYTESPERSAMPLE;
}

void initEDFInputIterator(struct EDFInputIterator *edfi, const struct EDFDecodedConfig *cfg) {
	edfi->cfg = *cfg;
	edfi->dataRecordNum = 0;
	edfi->sampleNum = 0;
}

int stepEDFInputIterator(struct EDFInputIterator *edfi)
{
	edfi->sampleNum += 1;
	if (edfi->sampleNum == edfi->cfg.chan[0].sampleCount) {
		edfi->sampleNum = 0;
		edfi->dataRecordNum += 1;
	}
}

/* Assumes all channels sample at the same frequency */
int fetchSamples(const struct EDFInputIterator *edfi, short *samples, FILE *fp)
{
	int retval;
	int i;
	retval = fseek(fp, 
			edfi->cfg.hdr.headerRecordBytes + 
			getDataRecordChunkSize(&edfi->cfg) * edfi->dataRecordNum +
			BYTESPERSAMPLE * edfi->sampleNum, SEEK_SET);
	if (retval != 0) return retval;
	retval = fseek(fp, edfi->sampleNum * BYTESPERSAMPLE, SEEK_CUR);
	if (retval != 0) return retval;
	for (i = 0; i < edfi->cfg.hdr.dataRecordChannels; ++i) {
		// TODO: Make this big-endian-safe and faster
		retval = fread(samples+i, 1, BYTESPERSAMPLE, fp);
		if (retval != BYTESPERSAMPLE) return 1;
		fseek(fp, (edfi->cfg.chan[i].sampleCount-1) * BYTESPERSAMPLE, SEEK_CUR);
	}
	return 0;
}

void printHeader(const struct EDFDecodedHeader *hdr)
{
	printf("The data record count is %d\n", hdr->dataRecordCount);
	printf("The data record channels is %d\n", hdr->dataRecordChannels);
	printf("The data record seconds is %f\n", hdr->dataRecordSeconds);
}

