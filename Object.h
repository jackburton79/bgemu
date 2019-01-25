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
class TileCell;
class Action;
class Object : public Referenceable {
public:
	Object(const char* name, const char* scriptName = NULL);

	void Print() const;

	const char* Name() const;
	void SetName(const char* name);

	virtual IE::point Position() const;
	virtual IE::rect Frame() const = 0;

	virtual void Clicked(Object* clicker);
	virtual void ClickedOn(Object* target);

	void EnteredRegion(Region* region);
	void ExitedRegion(Region* region);

	bool IsVisible() const;
	bool IsInsideVisibleArea() const;

	void SetVariable(const char* name, int32 value);
	int32 GetVariable(const char* name);

	virtual void Update(bool scripts);

	void SetScript(::Script* script);
	::Script* Script() const;

	virtual IE::point NearestPoint(const IE::point& point) const;
	
	void NewScriptRound();

	void SetStale(bool stale);
	bool IsStale() const;

protected:
	virtual ~Object();
	void LastReferenceReleased();

private:
	void _UpdateTileCell();

	std::string fName;
	::Script* fScript;
	uint16 fTicks;
	bool fVisible;

	std::map<std::string, uint32> fVariables;

	Region* fRegion;
	bool fStale;
};

#endif // __OBJECT_H
