#include <assert.h>
#include <openedf.h>

void printHeader(const struct EDFDecodedHeader *hdr)
{
	printf("The data record count is %d\n", hdr->dataRecordCount);
	printf("The data record channels is %d\n", hdr->dataRecordChannels);
	printf("The data record seconds is %f\n", hdr->dataRecordSeconds);
}

void printChannelHeader(const struct EDFDecodedChannelHeader *chdr)
{
	printf("The number of samples is %d\n", chdr->sampleCount);
	printf("The channel name is %s\n", chdr->label);
}

int main(int argc, char **argv)
{
	struct EDFDecodedConfig cfg;
	struct EDFInputIterator *edfi;
	FILE *fp;
	int i;
	int retval;
	int chunksize;
	assert(sizeof(struct EDFPackedHeader) == 256);
	assert(sizeof(struct EDFPackedChannelHeader) == 256);
	fp = fopen(argc<2?"SampleEEG.edf":argv[1], "rb");
	assert(fp != NULL && "Cannot open file!");
	retval = readEDF(fileno(fp), &cfg);
	printHeader(&cfg.hdr);
	printf("There are %f samples per second\n", getSamplesPerSecond(&cfg, 0));
	chunksize = getDataRecordChunkSize(&cfg);
	for (i = 0; i < cfg.hdr.dataRecordChannels; ++i) {
		printChannelHeader(&cfg.chan[i]);
	}

	printf("The chunk size is %d\n", chunksize);
	printf("The header size is %d\n", cfg.hdr.headerRecordBytes);
	edfi = newEDFInputIterator(&cfg);
	i = 0;
	for (;;) {
		int retval;
		short samples[MAXCHANNELS];
		retval = fetchSamples(edfi, samples, fp);
		if (retval != 0) break;
		i += 1;
		if (i % 1000 == 0) {
			printf("Read %d timesamples and on datarecord %05d:%04d\n", i, edfi->dataRecordNum, edfi->sampleNum);
		}
		stepEDFInputIterator(edfi);
	}

	fclose(fp);
	freeEDFInputIterator(edfi);
	return 0;
}
