/*
 * Referenceable.cpp
 *
 *  Created on: 12/lug/2012
 *      Author: stefano
 */

#include "Referenceable.h"

Referenceable::Referenceable(bool manualDelete)
	:
	fRefCount(0)
{
}


Referenceable::~Referenceable()
{
}


void
Referenceable::Acquire()
{
	fRefCount++;
}


bool
Referenceable::Release()
{
	if (--fRefCount <= 0)
		return true;

	return false;
}


int32
Referenceable::RefCount() const
{
	return fRefCount;
}
