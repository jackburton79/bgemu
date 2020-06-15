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
	virtual ~WorldMap();
	
	virtual GFX::rect AreaRect() const;

	virtual void Draw(Bitmap *surface);
	virtual void Clicked(uint16 x, uint16 y);
	virtual void MouseOver(uint16 x, uint16 y);

	void ActorEnteredArea(const Actor* actor);
	void ActorExitedArea(const Actor* actor);
	
	void ToggleConsole();

	virtual void ShowGUI();
	virtual void HideGUI();
	virtual bool IsGUIShown() const;

	virtual void VideoAreaChanged(uint16 width, uint16 height);

private:
	void _DrawConsole();
	GFX::rect _ConsoleRect() const;

	void _UnloadWorldMap();

	// WorldMap
	WMAPResource* fWorldMap;
	MOSResource* fWorldMapBackground;
	Bitmap*	fWorldMapBitmap;
	AreaEntry* fAreaUnderMouse;
};


#endif
