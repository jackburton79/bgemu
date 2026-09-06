/*
 * SoundEngine.h
 *
 *  Created on: 07/giu/2012
 *      Author: stefano
 */

#ifndef SOUNDENGINE_H_
#define SOUNDENGINE_H_

#include "SupportDefs.h"

#include <vector>

// One slot of SoundEngine's one-shot sound-effect pool - see PlaySample().
// `device` is really an SDL_AudioDeviceID (a plain uint32 typedef); kept
// as uint32 here rather than pulling SDL.h into this header, which every
// other file including SoundEngine.h (Core.cpp, MoviePlayer.cpp, ...)
// would otherwise inherit.
struct OneShotSound {
	uint32 device = 0;
	std::vector<uint8> data;
	uint32 position = 0;
	bool active = false;
	uint16 channels = 0;
	uint16 bitsPerSample = 0;
	uint32 sampleRate = 0;
};


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
	static bool Initialize();
	static void Destroy();

	static SoundEngine* Get();

	bool InitBuffers(bool stereo, bool bit16, uint16 sampleRate, uint32 bufferLen);
	void DestroyBuffers();
	SoundBuffer* Buffer();

	bool Lock();
	void Unlock();

	void StartStopAudio();
	bool IsPlaying();

	// Plays `data` (already-decoded raw PCM) once, on its own SDL audio
	// device - unlike the single continuous ring buffer above (built for
	// MVE movie audio, one stream at a time), this can overlap with the
	// movie stream and with other PlaySample() calls: each of up to
	// kMaxOneShots simultaneous one-shots gets its own device. Reuses an
	// idle pool slot already opened for the same format when possible;
	// if every slot is busy, steals the first one (interrupting whatever
	// it was playing) rather than dropping the new sound.
	void PlaySample(const uint8* data, uint32 dataSize, uint16 channels,
		uint16 bitsPerSample, uint32 sampleRate);

private:
	SoundBuffer* fBuffer;
	bool fPlaying;

	static const uint8 kMaxOneShots = 8;
	OneShotSound fOneShots[kMaxOneShots];

	SoundEngine();
	~SoundEngine();

	static void MixAudio(void *unused, uint8 *stream, int len);
	static void MixOneShot(void* userData, uint8* stream, int len);
};

#endif /* SOUNDENGINE_H_ */
