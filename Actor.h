#ifndef __ACTOR_H
#define __ACTOR_H

#include "IETypes.h"

#include <SDL.h>

enum orientation {
	ORIENTATION_S	= 0,
	ORIENTATION_SW	= 1,
	ORIENTATION_W 	= 2,
	ORIENTATION_NW	= 3,
	ORIENTATION_N	= 4
};


class Animation;
class CREResource;
class Actor {
public:
	Actor(::actor &actor);
	~Actor();

	const char *Name() const;

	point Position() const;
	orientation Orientation() const;
	point Destination() const;

	CREResource *CRE();

	void Draw(SDL_Surface *surface, SDL_Rect area);

	static res_ref AnimationFor(Actor &actor);
private:
	actor *fActor;
	CREResource *fCRE;
	Animation *fAnimation;
};

#endif //__ACTOR_H
