/*
 * ACMStream.cpp - see ACMStream.h
 */

#include "ACMStream.h"

#include <algorithm>
#include <fstream>
#include <iterator>

ACMStream::ACMStream()
{
}


/* static */
ACMStream*
ACMStream::Open(const std::string& path)
{
	std::ifstream file(path, std::ios::binary);
	if (!file)
		return nullptr;

	ACMStream* stream = new ACMStream();
	stream->fFile.assign(std::istreambuf_iterator<char>(file),
		std::istreambuf_iterator<char>());
	if (!stream->fDecoder.Open(stream->fFile.data(), stream->fFile.size())) {
		delete stream;
		return nullptr;
	}
	return stream;
}


uint16
ACMStream::Channels() const
{
	return fDecoder.Channels();
}


uint32
ACMStream::SampleRate() const
{
	return fDecoder.SampleRate();
}


size_t
ACMStream::Read(uint8* buffer, size_t bytes)
{
	const size_t samples = fDecoder.Read(reinterpret_cast<int16*>(buffer),
		bytes / sizeof(int16));
	return samples * sizeof(int16);
}


uint32
ACMStream::DurationMs() const
{
	const uint32 frames = fDecoder.TotalSamples() / std::max<uint16>(fDecoder.Channels(), 1);
	return (uint32)((uint64)frames * 1000 / std::max<uint32>(fDecoder.SampleRate(), 1));
}
