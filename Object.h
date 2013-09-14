/*
 * Scriptable.h
 *
 *  Created on: 12/lug/2012
 *      Author: stefano
 */

#ifndef __SCRIPTABLE_H
#define __SCRIPTABLE_H

#include "SupportDefs.h"

#include <list>
#include <map>
#include <string>
#include <vector>

// TODO: Actor and Object aren't well separated.
// Either merge in one class or improve the separation
struct object_node;
class Actor;
class Object;
class Region;
class Script;
class ScriptResults {
public:
	ScriptResults();
	const std::vector<Object*> Attackers() const;
	const std::vector<Object*> Hitters() const;
	const std::vector<Object*> EnteredActors() const;

	int32 CountAttackers() const;
	Object* AttackerAt(int32 i) const;
	Object* LastAttacker() const;
	Object* Clicker() const;
	int Shouted() const;

	std::string fOpenedBy;
	std::string fClosedBy;
	std::string fDetectedBy;

private:
	friend class Object;
	friend class Actor;
	std::vector<std::string> fAttackers;
	std::vector<std::string> fHitters;
	std::vector<std::string> fSeenBy;
	std::vector<std::string> fSeenList;

	// For Trigger Regions
	std::vector<std::string> fEnteredActors;

	Object* fClicker;

	int fShouted;
};


class Action;
class Object {
public:
	Object(const char* name, const char* scriptName = NULL);
	virtual ~Object();

	void Print() const;

	const char* Name() const;
	void SetName(const char* name);

	virtual IE::point Position() const;

	virtual void Clicked(Object* clicker);
	virtual void ClickedOn(Object* target);

	void EnteredRegion(Region* region);
	void ExitedRegion(Region* region);

	void SetSeenBy(Actor* actor);

	bool IsVisible() const;

	void SetVariable(const char* name, int32 value);
	int32 GetVariable(const char* name);

	void AddAction(Action* action);
	bool IsActionListEmpty() const;
	void ClearActionList();

	void Update(bool scripts);

	void SetScript(Script* script);

	virtual IE::point NearestPoint(const IE::point& point) const;

	Object* ResolveIdentifier(const int identifier) const;

	bool MatchWithOneInList(const std::vector<Object*>& vector) const;
	bool MatchNode(object_node* node) const;

	static Object* GetMatchingObjectFromList(
										const std::vector<Object*>&,
										object_node* node);

	static bool CheckIfNodesInList(const std::vector<object_node*>& nodeList,
						const std::vector<Object*>& objectList);

	static bool IsDummy(const object_node* node);

	bool IsEqual(const Object* object) const;
	bool IsEnemyOf(const Object* object) const;

	bool IsName(const char* name) const;
	bool IsClass(int c) const;
	bool IsRace(int race) const;
	bool IsGender(int gender) const;
	bool IsGeneral(int general) const;
	bool IsSpecific(int specific) const;
	bool IsAlignment(int alignment) const;
	bool IsEnemyAlly(int ea) const;
	void SetEnemyAlly(int ea);
	bool IsState(int state) const;

	void AttackTarget(Object* object);

	void NewScriptRound();

	void SetStale(bool stale);
	bool IsStale() const;

	ScriptResults* CurrentScriptRoundResults() const;
	ScriptResults* LastScriptRoundResults() const;

private:
	const char* fName;
	Script* fScript;
	uint16 fTicks;
	bool fVisible;

	ScriptResults* fCurrentScriptRoundResults;
	ScriptResults* fLastScriptRoundResults;

	std::map<std::string, uint32> fVariables;
	std::list<Action*> fActions;

	Region* fRegion;
	bool fStale;
};

#endif // __SCRIPTABLE_H
