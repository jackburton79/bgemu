/*
 * SoundEngine.h
 *
 *  Created on: 07/giu/2012
 *      Author: stefano
 */

#ifndef SOUNDENGINE_H_
#define SOUNDENGINE_H_

#include "SupportDefs.h"

class SoundEngine {
public:
	SoundEngine();
	~SoundEngine();

	static SoundEngine* Get();
	bool InitBuffers(bool stereo, bool bit16, uint16 sampleRate, uint16 bufferLen);
	void StartAudio();
	void StopAudio();
	bool IsPlaying();
};

#endif /* SOUNDENGINE_H_ */
