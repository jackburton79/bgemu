/*
 * SoundEngine.cpp
 *
 *  Created on: 07/giu/2012
 *      Author: stefano
 */

#include "SoundEngine.h"
#include "SDL.h"

#include <algorithm>

static SoundEngine* sSoundEngine = NULL;

SoundEngine::SoundEngine()
	:
	fBuffer(NULL),
	fPlaying(false)
{
	SDL_InitSubSystem(SDL_INIT_AUDIO);

}

SoundEngine::~SoundEngine()
{
	SDL_PauseAudio(1);
	SDL_CloseAudio();
	delete fBuffer;
}


/* static */
SoundEngine*
SoundEngine::Get()
{
	if (sSoundEngine == NULL)
		sSoundEngine = new SoundEngine();
	return sSoundEngine;
}


bool
SoundEngine::InitBuffers(bool stereo, bool bit16, uint16 sampleRate, uint32 bufferLen)
{
	uint32 length = 160000;
	printf("InitBuffers(%s, %s, %d, %d)\n",
			stereo ? "STEREO" : "MONO", bit16 ? "16BIT" : "8BIT", sampleRate, length);

	fBuffer = new SoundBuffer(stereo, bit16, sampleRate, length); // bufferLen);

	SDL_AudioSpec fmt;
	fmt.freq = sampleRate;
	fmt.format = AUDIO_S16;
	fmt.channels = stereo ? 2 : 1;
	fmt.samples = 512;
	fmt.callback = SoundEngine::MixAudio;
	fmt.userdata = this;

	if (SDL_OpenAudio(&fmt, NULL) < 0 ) {
		fprintf(stderr, "Unable to open audio: %s\n", SDL_GetError());
		exit(1);
	}

	SDL_PauseAudio(0);
	return true;
}


SoundBuffer*
SoundEngine::Buffer()
{
	return fBuffer;
}


void
SoundEngine::StartStopAudio()
{
	//printf("StartStopAudio(%d)\n", fPlaying ? 1 : 0);
	// TODO: Means "lock audio", actually
	if (fPlaying)
		SDL_LockAudio();
	else
		SDL_UnlockAudio();
	fPlaying = !fPlaying;
}


bool
SoundEngine::IsPlaying()
{
	return fPlaying;
}


/* static */
void
SoundEngine::MixAudio(void *castToThis, Uint8 *stream, int len)
{
	SoundEngine* engine = (SoundEngine*)castToThis;
	SoundBuffer* buffer = engine->Buffer();

	uint8* data = buffer->ConsumeSamples((uint16*)&len);
	if (len > 0)
		SDL_MixAudio(stream, data, len, SDL_MIX_MAXVOLUME);
}


// SoundBuffer
SoundBuffer::SoundBuffer(bool stereo, bool bit16, uint16 sampleRate, uint32 bufferLen)
	:
	fStereo(stereo),
	f16Bit(bit16),
	fSampleRate(sampleRate),
	fData(NULL),
	fBufferLength(bufferLen),
	fBufferPos(0),
	fConsumedPos(0)
{
	fData = (uint8*)malloc(bufferLen);
}


SoundBuffer::~SoundBuffer()
{
	free(fData);
}


bool
SoundBuffer::IsStereo() const
{
	return fStereo;
}


bool
SoundBuffer::Is16Bit() const
{
	return f16Bit;
}


uint16
SoundBuffer::SampleRate() const
{
	return fSampleRate;
}


uint8*
SoundBuffer::Data()
{
	return fData;
}


void
SoundBuffer::AddSample(sint16 sample)
{
	memcpy(fData + fBufferPos, &sample, sizeof(sample));
	fBufferPos += sizeof(sample);

	if (fBufferPos == fBufferLength)
		fBufferPos = 0;

	if (fBufferPos == fConsumedPos)
		throw "Buffer Overflow";
}


uint8*
SoundBuffer::ConsumeSamples(uint16 *numSamples)
{
	uint32 numRequested = (uint32)*numSamples;
	uint32 numAvailable = std::min(numRequested, AvailableData());

	uint8* data = &fData[fConsumedPos];
	fConsumedPos += numAvailable;
	if (fConsumedPos >= fBufferLength)
		fConsumedPos = (fConsumedPos - fBufferLength);

	*numSamples = numAvailable;

	return data;
}


uint32
SoundBuffer::AvailableData() const
{
	if (fBufferPos < fConsumedPos)
		return (fBufferLength - fConsumedPos) + fBufferPos;

	return fBufferPos - fConsumedPos;
}
