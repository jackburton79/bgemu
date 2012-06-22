/*
 * SoundEngine.h
 *
 *  Created on: 07/giu/2012
 *      Author: stefano
 */

#ifndef SOUNDENGINE_H_
#define SOUNDENGINE_H_

#include "SupportDefs.h"

class SoundBuffer {
public:
	SoundBuffer(bool stereo, bool bit16, uint16 sampleRate, uint32 bufferLen);
	~SoundBuffer();

	bool IsStereo() const;
	bool Is16Bit() const;
	uint16 SampleRate() const;
	uint8* Data();

	uint16 ConsumeSamples(uint8* destBuffer, uint16 numSamples);
	void AddSample(sint16 sample);

	uint32 AvailableData() const;

private:
	bool fStereo;
	bool f16Bit;
	uint16 fSampleRate;
	uint8* fData;
	uint32 fBufferLength;
	uint32 fBufferPos;
	uint32 fConsumedPos;
};


class SoundEngine {
public:
	SoundEngine();
	~SoundEngine();

	static SoundEngine* Get();
	bool InitBuffers(bool stereo, bool bit16, uint16 sampleRate, uint32 bufferLen);
	SoundBuffer* Buffer();

	void StartStopAudio();
	bool IsPlaying();

private:
	SoundBuffer* fBuffer;
	bool fPlaying;

	static void MixAudio(void *unused, uint8 *stream, int len);
};

#endif /* SOUNDENGINE_H_ */
