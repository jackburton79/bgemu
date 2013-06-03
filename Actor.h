#ifndef __ACTOR_H
#define __ACTOR_H

#include "IETypes.h"

//#include "Bitmap.h"
#include "Frame.h"
#include "Object.h"

#include <list>
#include <map>
#include <string>
#include <vector>

const static uint32 kNumAnimations = 8;
const static uint32 kNumActions = 2;

class ActionList;
class Animation;
class AnimationFactory;
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

	virtual const char *Name() const;
	CREResource *CRE() const;

	::Frame Frame() const;

	IE::orientation Orientation() const;
	void SetOrientation(IE::orientation o);

	IE::point Position() const;

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

	void MergeScripts();

	void SetVariable(const char* name, int32 value);
	int32 GetVariable(const char* name);

	::ActionList* ActionList();
	bool IsActionListEmpty() const;

	bool SkipConditions() const;
	void StopCheckingConditions();
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
	Animation* fAnimations[kNumActions][kNumAnimations];
	Animation* fCurrentAnimation;

	CREResource *fCRE;
//	::Script* fScript;
	bool fOwnsActor;

	bool fDontCheckConditions;
	bool fIsInterruptable;

	bool fFlying;
	bool fEnemyOfEveryone;
	bool fSelected;

	PathFinder* fPath;
	int fSpeed;

	std::map<std::string, uint32> fVariables;

	static std::vector<Actor*> sActors;

	void _Init();
	void _LoadAnimations(int action);
	void _AddScript(const res_ref& scriptName);
	void _SetOrientation(const IE::point& nextPoint);
	bool _IsReachable(const IE::point& pt);

	static void BlitWithMask(Bitmap* source, Bitmap* dest,
			GFX::rect& rect, IE::polygon& polygonMask);

};

#endif //__ACTOR_H
