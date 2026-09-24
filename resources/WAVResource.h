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
	// to `samples`, which is not cleared first): a plain PCM RIFF/WAVE
	// resource, or WAVC/ACM-compressed audio (see IESDP's wavc_v1.htm,
	// ACMDecoder), which comes out as 16 bit PCM. Returns false (samples
	// left untouched) for anything else.
	bool DecodePCM(std::vector<uint8>& samples, uint16& channels,
		uint16& bitsPerSample, uint32& sampleRate);

private:
	virtual ~WAVResource();
	bool _DecodeACM(std::vector<uint8>& samples, uint16& channels,
		uint16& bitsPerSample, uint32& sampleRate);
};

#endif /* WAVRESOURCE_H_ */
