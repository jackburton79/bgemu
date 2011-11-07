#ifndef __ACTOR_H
#define __ACTOR_H

#include "Types.h"

class CREResource;
class Actor {
public:
	Actor(::actor &actor);
	~Actor();

	const char *Name() const;
	CREResource *CRE();

private:
	actor *fActor;
	CREResource *fCRE;
};

#endif //__ACTOR_H
