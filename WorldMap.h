#ifndef __WORLDMAP_H
#define __WORLDMAP_H

#include "RoomBase.h"

#include "Bitmap.h"
#include "IETypes.h"
#include "Listener.h"
#include "Object.h"

#include <vector>

class Actor;
class Animation;
class ARAResource;
class AreaEntry;
class BCSResource;
class BackMap;
class Bitmap;
class Container;
class Door;
class Frame;
class MapOverlay;
class MOSResource;
class Region;
class Script;
class TileCell;
class WEDResource;
class WMAPResource;
class WorldMap : public RoomBase {
public:
	WorldMap();
	~WorldMap();
	
	GFX::rect AreaRect() const;

	virtual void Draw(Bitmap *surface);
	virtual void Clicked(uint16 x, uint16 y);
	virtual void MouseOver(uint16 x, uint16 y);

	void ActorEnteredArea(const Actor* actor);
	void ActorExitedArea(const Actor* actor);

	uint16 TileNumberForPoint(const IE::point& point);
	
	void ToggleConsole();
	void ToggleGUI();

	virtual void VideoAreaChanged(uint16 width, uint16 height);

private:

	void _DrawConsole();
	GFX::rect _ConsoleRect() const;

	void _UpdateCursor(int x, int y, int scrollByX, int scrollByY);

	void _UnloadWorldMap();

	// WorldMap
	WMAPResource* fWorldMap;
	MOSResource* fWorldMapBackground;
	Bitmap*	fWorldMapBitmap;

	int32 fMapHorizontalRatio;
	int32 fMapVerticalRatio;

	Reference<Object> fMouseOverObject;
};


#endif
