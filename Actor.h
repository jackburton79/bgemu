#ifndef __ACTOR_H
#define __ACTOR_H

#include "Types.h"

class CREResource;
class Actor {
public:
	Actor(::actor &actor);
	~Actor();

	point Position() const;
	const char *Name() const;
	CREResource *CRE();

	static res_ref AnimationFor(Actor &actor);
private:
	actor *fActor;
	CREResource *fCRE;
};

#endif //__ACTOR_H
