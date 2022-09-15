/*
 * Referenceable.cpp
 *
 *  Created on: 12/lug/2012
 *      Author: stefano
 */

#include "Referenceable.h"

#include <assert.h>
#include <iostream>

#define ACTORDEBUG 0
#if ACTORDEBUG
#include "Actor.h"
#endif

bool Referenceable::sDebug = false;

Referenceable::Referenceable(int32 initialRefCount)
	:
	fRefCount(initialRefCount)
{
	if (sDebug) {
		std::cout << "Referenceable::Referenceable(" << fRefCount;
		std::cout << ")" << std::endl;
	}
}


Referenceable::~Referenceable()
{
	if (sDebug) {
		std::cout << "Referenceable::~Referenceable(" << fRefCount;
		std::cout << ")" << std::endl;
	}
}


void
Referenceable::Acquire()
{
#if ACTORDEBUG
if (Actor* actor = dynamic_cast<Actor*>(this)) {
	std::cout << actor->Name() << "(" << actor->LongName() << ")";
	std::cout << "(" << actor->GlobalID() << ")";
	std::cout << ") - Referenceable::Acquire()" << std::endl;
}
#endif
	int previousRefCount = fRefCount++;
	if (sDebug) {
		std::cout << "Referenceable::Acquire(): ";
		std::cout << "previous:" << previousRefCount;
		std::cout << ", current: " << fRefCount << std::endl;
	}
	if (previousRefCount == 0)
		FirstReferenceAcquired();


}


bool
Referenceable::Release()
{
	int previousRefCount = fRefCount--;
	if (sDebug) {
		std::cout << "Referenceable::Release(): ";
		std::cout << "previous:" << previousRefCount;
		std::cout << ", current: " << fRefCount << std::endl;
	}
#if ACTORDEBUG
if (Actor* actor = dynamic_cast<Actor*>(this)) {
	std::cout << actor->Name() << "(" << actor->LongName() << ")";
	std::cout << "(" << actor->GlobalID() << ")";
	std::cout << "- Referenceable::Release()" << std::endl;
}
#endif
	int currentRefCount = fRefCount;
	if (previousRefCount == 1)
		LastReferenceReleased();
	
	if (currentRefCount <= 0)
		return true;

	return false;
}


int32
Referenceable::RefCount() const
{
	return fRefCount;
}


/* static */
void
Referenceable::SetDebug(bool debug)
{
	sDebug = debug;
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


// AutodeletingReferenceable
AutoDeletingReferenceable::AutoDeletingReferenceable()
	:
	Referenceable(1)
{
}


/* virtual */
AutoDeletingReferenceable::~AutoDeletingReferenceable()
{
}


/* virtual */
void
AutoDeletingReferenceable::LastReferenceReleased()
{
	delete this;
}


void
ReleaseAndNil(Referenceable*& ref)
{
	if (ref != NULL) {
		ref->Release();
		ref = NULL;
	}
}
