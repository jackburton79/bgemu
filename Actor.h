#ifndef __ACTOR_H
#define __ACTOR_H

#include "IETypes.h"

#include <SDL.h>

#include <vector>

enum orientation {
	ORIENTATION_S	= 0,
	ORIENTATION_SW	= 1,
	ORIENTATION_W 	= 2,
	ORIENTATION_NW	= 3,
	ORIENTATION_N	= 4,
	ORIENTATION_NE	= 5,
	ORIENTATION_E	= 6,
	ORIENTATION_SE	= 7
};


class Animation;
class BCSResource;
class CREResource;
class Script;
class Actor {
public:
	Actor(::actor &actor);
	Actor(const char* creName, point position, int face);
	~Actor();

	const char *Name() const;

	point Position() const;
	orientation Orientation() const;
	point Destination() const;
	void SetDestination(const point &dest);

	CREResource *CRE();

	void Draw(SDL_Surface *surface, SDL_Rect area);

	void SetScript(Script *script);
	::Script* Script();


	void UpdateMove();

	static res_ref AnimationFor(Actor &actor);

	// Global list of actors
	static void Add(Actor *a);
	static void Remove(const char* name);
	static Actor* GetByIndex(uint32 i);
	static Actor* GetByName(const char* name);
	static std::vector<Actor*>& List();

private:
	actor *fActor;
	Animation *fAnimation;
	CREResource *fCRE;
	BCSResource* fBCSResource;
	bool fOwnsActor;

	static std::vector<Actor*> sActors;

	void _Init();
};

#endif //__ACTOR_H
