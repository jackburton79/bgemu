/*
 * ACMStream.h
 *
 * A music track (a loose .acm file) as an AudioStream: the compressed file is
 * read into memory and decoded on demand by the audio thread.
 */

#pragma once

#include "ACMDecoder.h"
#include "AudioStream.h"

#include <string>
#include <vector>

class ACMStream : public AudioStream {
public:
	// NULL if the file can't be read or isn't ACM audio.
	static ACMStream* Open(const std::string& path);

	virtual uint16 Channels() const;
	virtual uint32 SampleRate() const;
	virtual size_t Read(uint8* buffer, size_t bytes);

	// Length of the track in milliseconds.
	uint32 DurationMs() const;

private:
	ACMStream();

	std::vector<uint8> fFile;
	ACMDecoder fDecoder;
};
