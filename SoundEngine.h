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
	SoundBuffer(bool stereo, bool bit16, uint16 sampleRate, uint16 bufferLen);
	~SoundBuffer();

	bool IsStereo() const;
	bool Is16Bit() const;
	uint16 SampleRate() const;
	uint16* Data();

private:
	bool fStereo;
	bool f16Bit;
	uint16 fSampleRate;
	uint16* fData;
	uint16 fBufferLength;
};


class SoundEngine {
public:
	SoundEngine();
	~SoundEngine();

	static SoundEngine* Get();
	bool InitBuffers(bool stereo, bool bit16, uint16 sampleRate, uint16 bufferLen);
	SoundBuffer* Buffer();

	void StartStopAudio();
	bool IsPlaying();

private:
	SoundBuffer* fBuffer;
	bool fPlaying;

	static void MixAudio(void *unused, uint8 *stream, int len);
};

#endif /* SOUNDENGINE_H_ */
