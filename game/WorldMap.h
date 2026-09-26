#ifndef __WORLDMAP_H
#define __WORLDMAP_H

#include "RoomBase.h"

#include "Bitmap.h"
#include "IETypes.h"
#include "Object.h"
#include "WMAPResource.h"

#include <vector>

class ARAResource;
class AreaEntry;
class Frame;
class MOSResource;
class Region;
class WEDResource;
class WMAPResource;
class WorldMap : public RoomBase {
public:
	// previousArea/direction: the area/edge the party just left through
	// (see SearchMap::EdgeDirection()) - used to reveal that area's
	// AREA_VISIBLE_FROM_ADJACENT-flagged neighbors on that side (see
	// _RevealAdjacentAreas()). Default (empty name, direction -1) is a
	// manual open (HUD button/hotkey) - reveals nothing new.
	WorldMap(const res_ref& previousArea = res_ref(), int direction = -1);

	virtual GFX::rect AreaRect() const;

	virtual void Draw();
	virtual void MouseDown(IE::point point);
	virtual void MouseMoved(IE::point point, uint32 transit);

	void ToggleConsole();

	virtual void Unload();
	virtual void ReloadArea();

protected:
	virtual ~WorldMap();

private:
	void _DrawConsole();
	GFX::rect _ConsoleRect() const;

	void _UnloadWorldMap();

	void _LoadAreaEntries();
	void _RevealAdjacentAreas(const res_ref& previousArea, int direction);
	AreaEntry* _CenterOnArea(const res_ref& areaName);
	bool _IsWalkable(const AreaEntry* area) const;
	bool _FindTravelLink(const AreaEntry* destination, arealink_entry& lastLink) const;
	void _MarkCurrentArea(const AreaEntry* entry);

	// WorldMap
	WMAPResource* fWorldMap;
	MOSResource* fWorldMapBackground;
	Bitmap*	fWorldMapBitmap;

	std::vector<AreaEntry*> fAreaEntries;
	AreaEntry* fAreaUnderMouse;

	// The area the party is actually in (backgrounded in
	// Core::fPreviousRoom while this WorldMap is current) - MouseDown()
	// needs this to tell "travel elsewhere" from "go back to where I
	// already am" apart.
	res_ref fCurrentAreaName;
	// The edge the party left through (-1: none).
	int fDirection;
};


#endif
