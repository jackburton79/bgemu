/*
 * Referenceable.cpp
 *
 *  Created on: 12/lug/2012
 *      Author: stefano
 */

#include "Referenceable.h"

#include <assert.h>

Referenceable::Referenceable(int32 initialRefCount)
	:
	fRefCount(initialRefCount)
{
}


Referenceable::~Referenceable()
{
}


void
Referenceable::Acquire()
{
	int previousRefCount = fRefCount++;
	if (previousRefCount == 0)
		FirstReferenceAcquired();
}


bool
Referenceable::Release()
{
	int previousRefCount = fRefCount--;
	if (previousRefCount == 1)
		LastReferenceReleased();
	
	if (fRefCount <= 0)
		return true;

	return false;
}


int32
Referenceable::RefCount() const
{
	return fRefCount;
}


/* virtual */
void
Referenceable::FirstReferenceAcquired()
{
}


/* virtual */
void
Referenceable::LastReferenceReleased()
{
}


void
ReleaseAndNil(Referenceable*& ref)
{
	if (ref != NULL) {
		ref->Release();
		ref = NULL;
	}
}
