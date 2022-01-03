#ifndef __ROOMBASE_H
#define __ROOMBASE_H

#include "Bitmap.h"
#include "Control.h"
#include "GraphicsDefs.h"
#include "IETypes.h"
#include "Object.h"

#include <vector>

class AreaEntry;
class Bitmap;
class Region;
class RoomBase : public Object, public Control {
public:
	RoomBase();

	virtual IE::rect Frame() const;
	
	// AreaRect is the size of the complete area map, starting at 0,0
	virtual GFX::rect AreaRect() const = 0;

	GFX::rect ViewPort() const;

	// The area of the map which is visible on screen
	IE::rect VisibleMapArea() const;
	IE::point AreaOffset() const;
	IE::point AreaCenterPoint() const;
	void SetAreaOffset(const IE::point& point);
	void SetRelativeAreaOffset(int16 xDelta, int16 yDelta);
	void SetAreaOffsetCenter(const IE::point& point);
	
	IE::point CenteredOffset(const IE::point& point) const;
	IE::point LeftToppedOffset(const IE::point& point) const;
	
	void SanitizeOffsetLeftTop(IE::point& point) const;
	void SanitizeOffsetCenter(IE::point& point) const;
	
	void ConvertToArea(GFX::rect& rect);
	void ConvertToArea(IE::point& point);
	void ConvertFromArea(GFX::rect& rect);
	void ConvertFromArea(IE::point& point);

	virtual void Draw() = 0;
	virtual void MouseDown(IE::point point) = 0;
	virtual void MouseMoved(IE::point point, uint32 transit) = 0;
	
	virtual void ToggleOverlays();
	virtual void TogglePolygons();
	virtual void ToggleAnimations();
	virtual void ToggleSearchMap();
	void ToggleConsole();

	virtual void ShowGUI() = 0;
	virtual void HideGUI() = 0;
	virtual bool IsGUIShown() const = 0;
	void ToggleGUI();

	virtual void ReloadArea() = 0;

	virtual void VideoAreaChanged(uint16 width, uint16 height);

private:
	IE::point fAreaOffset;

protected:
	Control* fSavedControl;
	int32 fControlID;

	virtual ~RoomBase();
	void _DrawConsole();
	GFX::rect _ConsoleRect() const;
};


#endif // __ROOMBASE_H
