#ifndef __ROOMBASE_H
#define __ROOMBASE_H

#include "Bitmap.h"
#include "GraphicsDefs.h"
#include "IETypes.h"
#include "Listener.h"
#include "Object.h"

#include <vector>

class AreaEntry;
class Bitmap;
class Region;
class RoomBase : public Object, public Listener {
public:
	RoomBase();
	virtual ~RoomBase();

	virtual IE::rect Frame() const;
	virtual GFX::rect AreaRect() const = 0;

	GFX::rect ViewPort() const;
	void SetViewPort(GFX::rect rect);

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

	static bool IsPointPassable(const IE::point& point);

	virtual void ToggleOverlays();
	virtual void TogglePolygons();
	virtual void ToggleAnimations();
	virtual void ToggleSearchMap();
	void ToggleConsole();
	void ToggleGUI();
	virtual void ToggleDayNight();

	virtual void VideoAreaChanged(uint16 width, uint16 height);

protected:
	GFX::rect fScreenArea;
	GFX::rect fMapArea; // the part of map which is visible. It's fScreenArea
						// offsetted to fAreaOffset
	IE::point fAreaOffset;

private:
	void _DrawConsole();
	GFX::rect _ConsoleRect() const;

	void _UpdateCursor(int x, int y, int scrollByX, int scrollByY);
};


#endif // __ROOMBASE_H
