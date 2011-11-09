#ifndef __ACTOR_H
#define __ACTOR_H

#include "Types.h"

#include <SDL.h>

class CREResource;
class Actor {
public:
	Actor(::actor &actor);
	~Actor();

	void Draw(SDL_Surface *surface, SDL_Rect area);

	point Position() const;
	const char *Name() const;
	CREResource *CRE();

	static res_ref AnimationFor(Actor &actor);
private:
	actor *fActor;
	CREResource *fCRE;
};

#endif //__ACTOR_H
