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


class Bitmap;
class MovieDecoder;
// Parses the MVE container format only (chunks/stream opcodes, audio/
// video/palette/timer data) - playback (pacing, input, presenting frames
// to the screen) is MoviePlayer's job, driven by pumping
// DecodeNextChunk() in a loop. See game/MoviePlayer.h.
class MVEResource : public Resource {
public:
	MVEResource(const res_ref &name);
	static Resource* Create(const res_ref& name);

	// Decodes the next chunk in the stream. Returns false once there's
	// nothing left to decode (end of stream/data) - the caller should
	// stop calling it at that point.
	bool DecodeNextChunk();

	// True if a video frame became ready to display since the last call
	// (and resets it back to false) - set when the stream's own
	// "blit backbuffer" opcode is processed while decoding a chunk.
	bool ConsumeFrameReady();
	// The just-decoded frame, valid once ConsumeFrameReady() returns true.
	Bitmap* CurrentFrame() const;

	// Ticks between frames (0 until the stream's CREATE_TIMER opcode is
	// seen), and the Timer::Ticks() value the last frame became ready -
	// together, what MoviePlayer needs to pace playback the same way the
	// original single-class implementation did.
	uint32 FrameDelay() const;
	uint32 LastFrameTime() const;

private:
	virtual ~MVEResource();
	void DecodeChunk(chunk_header);
	bool ExecuteOpcode(op_stream_header opcode);
	void ReadAudioData(Stream* stream, uint16 numSamples);
	void AddSilence(uint16 numSamples);

	MovieDecoder *fDecoder;
	uint32 fTimer;
	uint32 fLastFrameTime;
	bool fFrameReady;
};

const char *chunktostr(chunk_header);
const char *opcodetostr(uint8);

#endif
