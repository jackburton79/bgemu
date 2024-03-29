#ifndef __WORLDMAP_H
#define __WORLDMAP_H

#include "RoomBase.h"

#include "Bitmap.h"
#include "IETypes.h"
#include "Listener.h"
#include "Object.h"

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
	WorldMap();
	
	virtual GFX::rect AreaRect() const;

	virtual void Draw();
	virtual void MouseDown(IE::point point);
	virtual void MouseMoved(IE::point point, uint32 transit);

	void ActorEnteredArea(const Actor* actor);
	void ActorExitedArea(const Actor* actor);
	
	void ToggleConsole();

	virtual void ShowGUI();
	virtual void HideGUI();
	virtual bool IsGUIShown() const;

	virtual void ReloadArea();

protected:
	virtual ~WorldMap();

private:
	void _DrawConsole();
	GFX::rect _ConsoleRect() const;

	void _UnloadWorldMap();

	void _LoadAreaEntries();

	// WorldMap
	WMAPResource* fWorldMap;
	MOSResource* fWorldMapBackground;
	Bitmap*	fWorldMapBitmap;

	std::vector<AreaEntry*> fAreaEntries;
	AreaEntry* fAreaUnderMouse;
};


#endif
