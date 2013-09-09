#ifndef __ACTOR_H
#define __ACTOR_H

#include "Bitmap.h"
#include "IETypes.h"
#include "Object.h"

#include <list>
#include <vector>

const static uint32 kNumAnimations = 8;
const static uint32 kNumActions = 2;

class Animation;
class AnimationFactory;
class Bitmap;
class BCSResource;
class CREResource;
class PathFinder;
class Script;

class Actor : public Object {
public:
	Actor(IE::actor& actor);
	Actor(IE::actor& actor, CREResource* cre);
	Actor(const char* creName, IE::point position, int face);
	virtual ~Actor();

	CREResource *CRE() const;

	const ::Bitmap* Bitmap() const;
	GFX::rect Frame() const;

	IE::orientation Orientation() const;
	void SetOrientation(IE::orientation o);

	IE::point Position() const;
	void SetPosition(const IE::point& position);

	IE::point Destination() const;
	void SetDestination(const IE::point &dest);

	void Shout(int number);

	void SetIsEnemyOfEveryone(bool enemy);
	bool IsEnemyOfEveryone() const;

	void SetFlying(bool fly);
	bool IsFlying() const;

	void Select(bool select);
	bool IsSelected() const;

	void SetInterruptable(const bool interrupt);
	bool IsInterruptable() const;

	Script* MergeScripts();

	bool SkipConditions() const;
	void StopCheckingConditions();
	void UpdateSee();
	void SetSeen(Object* object);
	bool HasSeen(const Object* object) const;
	void UpdateMove(bool ignoreBlocks);

	// Global list of actors
	static void Add(Actor *a);
	static void Remove(const char* name);
	static Actor* GetByIndex(uint32 i);
	static Actor* GetByName(const char* name);
	static std::vector<Actor*>& List();

private:
	IE::actor *fActor;
	AnimationFactory* fAnimationFactory;
	Animation* fCurrentAnimation;

	CREResource *fCRE;
	bool fOwnsActor;

	bool fDontCheckConditions;
	bool fIsInterruptable;

	bool fFlying;
	bool fEnemyOfEveryone;
	bool fSelected;

	PathFinder* fPath;
	int fSpeed;

	static std::vector<Actor*> sActors;

	void _Init();
	void _AddScript(Script*& destination, const res_ref& scriptName);
	void _SetOrientation(const IE::point& nextPoint);
	bool _IsReachable(const IE::point& pt);

};

#endif //__ACTOR_H
