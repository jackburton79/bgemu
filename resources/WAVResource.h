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

class WAVResource: public Resource {
public:
	WAVResource(const res_ref& name);

	virtual bool Load(Archive *archive, uint32 key);
	virtual void Dump();

private:
	virtual ~WAVResource();

};

#endif /* WAVRESOURCE_H_ */
