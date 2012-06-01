#ifndef __ACTOR_H
#define __ACTOR_H

#include "IETypes.h"

#include "Bitmap.h"

#include <vector>

class Animation;
class BCSResource;
class CREResource;
class Script;
class Actor {
public:
	Actor(IE::actor &actor);
	Actor(const char* creName, IE::point position, int face);
	~Actor();

	const char *Name() const;

	IE::point Position() const;
	IE::orientation Orientation() const;
	IE::point Destination() const;
	void SetDestination(const IE::point &dest);

	CREResource *CRE();

	void Draw(Bitmap *surface, GFX::rect area);

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
	IE::actor *fActor;
	Animation *fAnimation;
	CREResource *fCRE;
	BCSResource* fBCSResource;
	bool fOwnsActor;

	static std::vector<Actor*> sActors;

	void _Init();
};

#endif //__ACTOR_H
