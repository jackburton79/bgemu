/*
 * WAVResource.h
 *
 *      Author: Stefano Ceccherini
 */

#ifndef WAVRESOURCE_H_
#define WAVRESOURCE_H_

#include "Resource.h"

#include <map>
#include <string>
#include <vector>

class WAVResource: public Resource {
public:
	WAVResource(const res_ref& name);
	static Resource* Create(const res_ref& name);

	virtual bool Load(Archive *archive, uint32 key);
	virtual void Dump();

	// Decodes this resource's raw PCM sample data for playback (appended
	// to `samples`, which is not cleared first). Returns false (samples
	// left untouched) if this isn't a plain PCM RIFF/WAVE resource - most
	// importantly, for WAVC-wrapped ACM-compressed audio (see IESDP's
	// wavc_v1.htm; the loose voiceset files under a real install's
	// sounds/ folder are WAVC, while most in-BIF sound-effect resources
	// are plain RIFF/PCM) - no ACM decoder exists in this codebase.
	bool DecodePCM(std::vector<uint8>& samples, uint16& channels,
		uint16& bitsPerSample, uint32& sampleRate);

private:
	virtual ~WAVResource();
};

#endif /* WAVRESOURCE_H_ */
