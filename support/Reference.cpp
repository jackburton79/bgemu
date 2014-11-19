/*
 * Reference.cpp
 *
 *  Created on: 17/nov/2014
 *      Author: stefano
 */

#include "Reference.h"
#include "Referenceable.h"

Reference::	Reference(Referenceable* target)
	:
	fTarget(target)
{
	fTarget->Acquire();
}


Reference::Reference(const Reference& ref)
	:
	fTarget(NULL)
{
	fTarget = const_cast<Reference&>(ref).Target();
	fTarget->Acquire();
}


Reference::~Reference()
{
	if (fTarget->Release())
		delete fTarget;
}


Referenceable*
Reference::Target()
{
	return fTarget;
}


Reference&
Reference::operator=(const Reference& ref)
{
	fTarget = const_cast<Reference&>(ref).Target();
	fTarget->Acquire();

	return *this;
}
