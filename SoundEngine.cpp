/*
 * SoundEngine.cpp
 *
 *  Created on: 07/giu/2012
 *      Author: stefano
 */

#include "SoundEngine.h"
#include "SDL.h"

static SoundEngine* sSoundEngine = NULL;

SoundEngine::SoundEngine()
	:
	fBuffer(NULL)
{
	SDL_InitSubSystem(SDL_INIT_AUDIO);

}

SoundEngine::~SoundEngine()
{
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
SoundEngine::InitBuffers(bool stereo, bool bit16, uint16 sampleRate, uint16 bufferLen)
{
	printf("InitBuffers(%s, %s, %d, %d)\n",
			stereo ? "STEREO" : "MONO", bit16 ? "16BIT" : "8BIT", sampleRate, bufferLen);
	fBuffer = new SoundBuffer(stereo, bit16, sampleRate, bufferLen);
	return true;
}


SoundBuffer*
SoundEngine::Buffer()
{
	return fBuffer;
}


void
SoundEngine::StartAudio()
{

}


void
SoundEngine::StopAudio()
{

}


bool
SoundEngine::IsPlaying()
{
	return false;
}


// SoundBuffer
SoundBuffer::SoundBuffer(bool stereo, bool bit16, uint16 sampleRate, uint16 bufferLen)
	:
	fStereo(stereo),
	f16Bit(bit16),
	fSampleRate(sampleRate),
	fData(NULL),
	fBufferLength(bufferLen)
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
