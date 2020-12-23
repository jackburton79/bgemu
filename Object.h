/*
 * Object.h
 *
 *  Created on: 12/lug/2012
 *      Author: stefano
 */

#ifndef __OBJECT_H
#define __OBJECT_H

#include "IETypes.h"
#include "Reference.h"
#include "Referenceable.h"
#include "SupportDefs.h"
#include "Variables.h"

#include <list>
#include <map>
#include <string>
#include <vector>

class Object;
struct trigger_entry {
	trigger_entry(std::string trigName);
	trigger_entry(std::string trigName, Object* targetObject);
	std::string trigger_name;
	uint16 target_id;
};

struct object_params;
struct trigger_node;
class Action;
class Actor;
class Region;
class Script;
class TileCell;
class Object : public Referenceable {
public:
	enum object_type {
		ACTOR,
		AREA,
		CONTAINER,
		DOOR,
		REGION
	};

	Object(const char* name, object_type objectType, const char* scriptName = NULL);

	void Print() const;

	const char* Name() const;
	void SetName(const char* name);

	object_type Type() const;

	uint16 GlobalID() const;
	void SetGlobalID(uint16 id);

	bool IsNew() const;

	virtual IE::rect Frame() const = 0;

	int32 GetVariable(const char* name) const;
	void SetVariable(const char* name, int32 value);

	void Clicked(Object* clicker);

	void EnteredRegion(Region* region);
	void ExitedRegion(Region* region);

	bool IsVisible() const;
	bool IsInsideVisibleArea() const;

	virtual void Update(bool scripts);

	void SetActive(bool active);
	bool IsActive() const;

	void AddAction(Action* action, bool now = false);
	void ExecuteActions();
	bool IsActionListEmpty() const;
	void ClearActionList();

	void AddTrigger(trigger_entry entry);
	bool HasTrigger(std::string trigName) const;
	bool HasTrigger(std::string trigName, trigger_node* triggerNode) const;
	Object* FindTrigger(std::string trigName) const;
	Object* LastTrigger() const;

	void PrintTriggers() const;
	void ClearTriggers();

	void SetInterruptable(const bool interrupt);
	bool IsInterruptable() const;

	void AddScript(::Script* script);
	void ClearScripts();

	void SetWaitTime(int32 time);
	
	virtual IE::point NearestPoint(const IE::point& start) const;
	
	void DestroySelf();
	bool ToBeDestroyed() const;

	static void SetDebug(bool debug);

protected:
	virtual ~Object();
	void LastReferenceReleased();

private:
	void _UpdateTileCell();
	void _ExecuteScripts(int32 maxLevel);
	void _ExecuteAction(Action& action);
	
	std::string fName;
	object_type fType;
	uint16 fGlobalID;

	int32 fTicks;
	int32 fTicksIdle;

	bool fVisible;
	bool fActive;
	bool fIsInterruptable;

	typedef std::vector< ::Script*> ScriptsList;
	ScriptsList fScripts;
	int32 fWaitTime;

	Action* fCurrentAction;
	std::list<Action*> fActions;
	std::list<trigger_entry> fTriggers;	
	Object* fLastTrigger;
	
	::Variables fVariables;

	Region* fRegion;

	bool fToDestroy;

	static bool sDebug;
};

#endif // __OBJECT_H
