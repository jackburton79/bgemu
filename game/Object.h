/*
 * Object.h
 *
 *  Created on: 12/lug/2012
 *      Author: stefano
 */

#ifndef __OBJECT_H
#define __OBJECT_H

#include "IETypes.h"
#include "Polygon.h"
#include "Reference.h"
#include "Referenceable.h"
#include "SupportDefs.h"
#include "Variables.h"

#include <list>
#include <map>
#include <string>
#include <vector>

class Object;

enum SCRIPT_LEVEL {
	SCRIPT_LEVEL_OVERRIDE = 0,
	SCRIPT_LEVEL_AREA = 1,
	SCRIPT_LEVEL_SPECIFICS = 2,
	SCRIPT_LEVEL_CLASS = 3,
	SCRIPT_LEVEL_RACE= 4,
	SCRIPT_LEVEL_GENERAL = 5,
	SCRIPT_LEVEL_DEFAULT = 6
};

struct trigger_entry {
	trigger_entry(const std::string& trigName);
	trigger_entry(const std::string& trigName, Object* targetObject);
	std::string trigger_name;
	uint16 target_id;
};


class Outline {
public:
	enum type {
		OUTLINE_RECT = 0,
		OUTLINE_POLY = 1
	};
	Outline(const IE::rect& rect, GFX::Color color);
	Outline(const ::Polygon& polygon, GFX::Color color);
	GFX::Color Color() const;
	int Type() const;
	IE::rect Rect() const;
	::Polygon Polygon() const;
private:
	GFX::Color fColor;
	int fType;
	IE::rect fRect;
	::Polygon fPolygon;
};


struct object_params;
struct trigger_params;
class Action;
class Actor;
class AreaRoom;
class Region;
class Script;
class SpellEffect;
class TileCell;
class Object : public AutoDeletingReferenceable {
public:
	enum object_type {
		ACTOR,
		AREA,
		CONTAINER,
		DOOR,
		REGION
	};

	Object(const char* name, object_type objectType, const char* scriptName = NULL);

	virtual void Print() const;

	const char* Name() const;
	void SetName(const char* name);

	object_type Type() const;

	uint16 GlobalID() const;
	void SetGlobalID(uint16 id);

	void SetArea(AreaRoom* area);
	AreaRoom* Area() const;

	bool IsNew() const;

	virtual IE::rect Frame() const = 0;
	virtual ::Outline Outline() const;

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

	void AddAction(Action* action);
	void ExecuteActions();
	bool IsActionListEmpty() const;
	Action* PopNextAction();
	void ClearCurrentAction();
	void ClearActionList();

	void AddTrigger(const trigger_entry& entry);
	bool HasTrigger(const std::string& trigName) const;
	bool HasTrigger(const std::string& trigName, trigger_params* triggerNode) const;
	Object* FindTrigger(const std::string& trigName) const;
	Object* LastTrigger() const;

	void PrintTriggers() const;
	void ClearTriggers();

	void SetInterruptable(const bool interrupt);
	bool IsInterruptable() const;

	void AddScript(::Script* script);
	void ClearScripts();

	void Disable();

	void AddSpellEffect(SpellEffect* effect);

	void SetWaitTime(int32 time);
	
	virtual IE::point NearestPoint(const IE::point& start) const;
	
	void DestroySelf();
	bool ToBeDestroyed() const;

	static void SetDebug(bool debug);

protected:
	virtual ~Object();

private:
	void _UpdateTileCell();
	void _HandleScripting(int32 maxLevel);
	void _ExecuteScripts(int32 maxLevel);
	void _ExecuteAction(Action& action);
	void _ApplySpellEffects();

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

	std::list<SpellEffect*> fSpellEffects;

	AreaRoom* fArea;
	Region* fRegion;

	bool fDisabled;
	bool fToDestroy;

	static bool sDebug;
};

#endif // __OBJECT_H
