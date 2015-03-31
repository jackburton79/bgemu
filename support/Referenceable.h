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
	Referenceable(int32 initialRefCount = 1);
	void Acquire();
	bool Release();

	int32 RefCount() const;

protected:

	virtual ~Referenceable();

	virtual void FirstReferenceAcquired();
	virtual void LastReferenceReleased();
	
private:
	int32 fRefCount;
};


extern void ReleaseAndNil(Referenceable*& );

#endif /* REFERENCEABLE_H_ */
