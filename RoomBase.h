#ifndef __ROOMBASE_H
#define __ROOMBASE_H

#include "Bitmap.h"
#include "GraphicsDefs.h"
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
class RoomContainer : public Object, public Listener {
public:
	ARAResource* AREA() const;

	virtual IE::rect Frame() const;

	GFX::rect ViewPort() const;
	void SetViewPort(GFX::rect rect);

	virtual GFX::rect AreaRect() const = 0;
	IE::point AreaOffset() const;
	GFX::rect VisibleArea() const;

	void SetAreaOffset(IE::point point);
	void SetRelativeAreaOffset(IE::point point);
	void CenterArea(const IE::point& point);

	void ConvertToArea(GFX::rect& rect);
	void ConvertToArea(IE::point& point);
	void ConvertFromArea(GFX::rect& rect);
	void ConvertFromArea(IE::point& point);

	void ConvertToScreen(IE::point& point);
	void ConvertToScreen(GFX::rect& rect);

	virtual void Draw(Bitmap *surface) = 0;
	virtual void Clicked(uint16 x, uint16 y) = 0;
	virtual void MouseOver(uint16 x, uint16 y) = 0;

	uint16 TileNumberForPoint(const IE::point& point);

	uint8 PointHeight(const IE::point& point) const;
	uint8 PointLight(const IE::point& point) const;
	uint8 PointSearch(const IE::point& point) const;

	static bool IsPointPassable(const IE::point& point);

	void ToggleOverlays();
	void TogglePolygons();
	void ToggleAnimations();
	void ToggleSearchMap();
	void ToggleConsole();
	void ToggleGUI();
	void ToggleDayNight();

	virtual void VideoAreaChanged(uint16 width, uint16 height);

	//static RoomContainer* Get();

	RoomContainer();
	~RoomContainer();

protected:
	GFX::rect fScreenArea;
	GFX::rect fMapArea; // the part of map which is visible. It's fScreenArea
						// offsetted to fAreaOffset
	IE::point fAreaOffset;

private:
	void _DrawConsole();
	GFX::rect _ConsoleRect() const;

	void _UpdateCursor(int x, int y, int scrollByX, int scrollByY);

	int32 fMapHorizontalRatio;
	int32 fMapVerticalRatio;

	int fDrawSearchMap;
	bool fDrawOverlays;
	bool fDrawPolygons;
	bool fDrawAnimations;
	bool fShowingConsole;
};


#endif // __ROOMBASE_H
