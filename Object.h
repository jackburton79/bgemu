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
#include <vector>

// TODO: Actor and Object aren't well separated.
// Either merge in one class or improve the separation
struct object_node;
class Actor;
class Object;
class Script;
class ScriptResults {
public:
	ScriptResults();
	const std::vector<Object*>& Attackers() const;
	const std::vector<Object*>& Hitters() const;
	const std::vector<Object*>& SeenList() const;

	int32 CountAttackers() const;
	Object* AttackerAt(int32 i) const;
	Object* LastAttacker() const;

	int Shouted() const;

private:
	friend class Object;
	friend class Actor;
	std::vector<Object*> fAttackers;
	std::vector<Object*> fHitters;
	std::vector<Object*> fSeen;

	int fShouted;
};


class Object {
public:
	Object(const char* name);
	virtual ~Object();

	void Print() const;

	virtual const char* Name() const;

	bool See(Object* object);
	bool IsVisible() const;

	void Update();

	void SetScript(Script* script);

	Object* ResolveIdentifier(const int identifier) const;

	bool MatchWithOneInList(const std::vector<Object*>& vector) const;
	//static bool MatchListWithList(const std::vector)
	bool MatchNode(object_node* node) const;

	static bool CheckIfNodeInList(object_node* node, const std::vector<Object*>& vector);
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

	void Attack(Object* object);

	void NewScriptRound();

	ScriptResults* CurrentScriptRoundResults() const;
	ScriptResults* LastScriptRoundResults() const;

protected:
	const char* fName;
	bool fVisible;

	Script* fScript;
	uint16 fTicks;

	ScriptResults* fCurrentScriptRoundResults;
	ScriptResults* fLastScriptRoundResults;
};

#endif // __SCRIPTABLE_H
