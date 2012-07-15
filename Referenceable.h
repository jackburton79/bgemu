/*
 * Referenceable.h
 *
 *  Created on: 12/lug/2012
 *      Author: stefano
 */

#include "SupportDefs.h"

#ifndef __REFERENCEABLE_H
#define __REFERENCEABLE_H

class Referenceable {
public:
	Referenceable();
	virtual ~Referenceable();

	void Acquire();
	bool Release();

private:
	int32 fRefCount;
};

#endif /* REFERENCEABLE_H_ */
