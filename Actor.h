#ifndef __ACTOR_H
#define __ACTOR_H

#include "IETypes.h"

#include "Bitmap.h"
#include "Object.h"

#include <list>
#include <map>
#include <string>
#include <vector>

const static uint32 kNumAnimations = 9;

class ActionList;
class Animation;
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

	const char *Name() const;
	CREResource *CRE();

	void Draw(GFX::rect area, Bitmap* heightMap);

	IE::orientation Orientation() const;
	IE::point Position() const;

	IE::point Destination() const;
	void SetDestination(const IE::point &dest);

	void SetFlying(bool fly);
	bool IsFlying() const;

	void SetInterruptable(const bool interrupt);
	bool IsInterruptable() const;

	Actor* LastAttacker() const;
	void Attack(Actor* target);

	void MergeScripts();
	void SetScript(Script *script);
	::Script* Script();

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
	Animation *fAnimations[kNumAnimations];
	CREResource *fCRE;
	::Script* fScript;
	bool fOwnsActor;

	bool fDontCheckConditions;
	bool fIsInterruptable;

	bool fFlying;

	PathFinder* fPath;
	Actor* fLastAttacker;

	std::map<std::string, uint32> fVariables;

	static std::vector<Actor*> sActors;

	void _Init();
	void _AddScript(const res_ref& scriptName);
	void _SetOrientation(const IE::point& nextPoint);
	bool _IsReachable(const IE::point& pt);

	static void BlitWithMask(Bitmap* source, Bitmap* dest,
			GFX::rect& rect, IE::polygon& polygonMask);

};

#endif //__ACTOR_H
