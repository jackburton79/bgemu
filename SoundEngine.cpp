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
{
	SDL_InitSubSystem(SDL_INIT_AUDIO);

}

SoundEngine::~SoundEngine()
{
	// TODO Auto-generated destructor stub
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
	return true;
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
