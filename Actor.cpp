#include "Actor.h"
#include "CreResource.h"
#include "ResManager.h"

Actor::Actor(::actor &actor)
	:
	fActor(&actor)
{
	fCRE = gResManager->GetCRE(fActor->cre);
}


Actor::~Actor()
{
	gResManager->ReleaseResource(fCRE);
}


const char *
Actor::Name() const
{
	return (const char *)fActor->name;
}


CREResource *
Actor::CRE()
{
	return fCRE;
}
