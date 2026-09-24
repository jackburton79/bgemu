/*
 * ACMDecoder.h
 *
 * Decoder for Interplay's ACM audio (the compression of the games' sounds and
 * music: WAVC resources and loose .acm files). Ported from the ACM reader of
 * GemRB (gemrb/plugins/ACMReader), itself derived from libacm - same
 * algorithm, same tables; the buffers are std::vector and the input is a memory
 * block instead of a stream.
 */

#ifndef ACMDECODER_H_
#define ACMDECODER_H_

#include "SupportDefs.h"

#include <vector>

class ACMDecoder {
public:
	ACMDecoder();

	// `data` is a WAVC resource (a 28 byte WAVC header, then the ACM stream) or
	// a bare ACM stream. Must stay valid while decoding.
	bool Open(const uint8* data, size_t size);

	uint16 Channels() const;
	uint32 SampleRate() const;
	// Number of 16 bit samples (all channels) in the stream.
	uint32 TotalSamples() const;

	// Decodes up to `count` 16 bit samples into `buffer`; returns how many
	// were decoded (fewer at the end of the stream, 0 on a damaged one).
	size_t Read(int16* buffer, size_t count);

	// Decodes the whole stream as interleaved little-endian 16 bit PCM.
	bool DecodeAll(std::vector<uint8>& pcm);

private:
	// One block of the stream: subblocks rows of (1 << levels) values.
	bool _UnpackBlock();
	void _SubbandDecode();

	// Bit reader.
	void _PrepareBits(int bits);
	int _GetBits(int bits);

	// The amplitude "fillers": each fills one column of the block.
	bool _Fill(int pass, int kind);
	void _Set(int pass, int row, int value);

	void _Step4d3fcc(int16* memory, int* buffer, int sbSize, int blocks);
	void _Step4d420c(int* memory, int* buffer, int sbSize, int blocks);

	const uint8* fData;
	size_t fSize;
	size_t fPos;

	uint16 fChannels;
	uint32 fSampleRate;
	uint32 fTotalSamples;
	uint32 fSamplesLeft;

	int fLevels;
	int fSubblocks;
	int fBlockSize;
	int fSubbandSize;

	uint32 fNextBits;
	int fAvailBits;

	std::vector<int16> fAmplitudes;	// 0x10000 entries, centered at 0x8000
	std::vector<int> fBlock;
	std::vector<int16> fFirstMemory;
	std::vector<int> fMemory;
	size_t fBlockPos;
	uint32 fSamplesReady;
};

#endif /* ACMDECODER_H_ */
