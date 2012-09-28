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

class Animation;
class BCSResource;
class CREResource;
class PathFinder;
class Script;
class Actor : public Object {
public:
	Actor(IE::actor &actor);
	Actor(const char* creName, IE::point position, int face);
	virtual ~Actor();

	const char *Name() const;

	IE::orientation Orientation() const;
	IE::point Position() const;

	IE::point Destination() const;
	void SetDestination(const IE::point &dest);

	CREResource *CRE();

	void Draw(GFX::rect area, Bitmap* heightMap);

	void MergeScripts();
	void SetScript(Script *script);
	::Script* Script();

	void SetVariable(const char* name, int32 value);
	int32 GetVariable(const char* name);

	bool SkipConditions() const;
	void StopCheckingConditions();
	void UpdateMove();

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
	PathFinder* fPath;

	std::map<std::string, uint32> fVariables;

	static std::vector<Actor*> sActors;

	void _Init();
	void _AddScript(const res_ref& scriptName);
	void _SetOrientation(const IE::point& nextPoint);
};

#endif //__ACTOR_H
