#ifndef __MVERESOURCE_H
#define __MVERESOURCE_H

#include "Resource.h"

struct chunk_header {
	uint16 length;
	uint16 type;
};


struct op_stream_header {
	uint16 length;
	uint8 type;
	uint8 version;
};


class MovieDecoder;
class MVEResource : public Resource {
public:
	MVEResource(const res_ref &name);
	virtual ~MVEResource();
	
	void Play();

private:
	bool GetNextChunk();
	void DecodeChunk(chunk_header);
	bool ExecuteOpcode(op_stream_header opcode);
	void ReadAudioData(Stream* stream, uint16 numSamples);
	void AddSilence(uint16 numSamples);
	MovieDecoder *fDecoder;
	uint32 fTimer;
	uint32 fLastFrameTime;
};

const char *chunktostr(chunk_header);
const char *opcodetostr(uint8);

#endif
