#include "RoomBase.h"

#include "Bitmap.h"
#include "Graphics.h"
#include "GraphicsEngine.h"
#include "GUI.h"
#include "RectUtils.h"

#include <algorithm>
#include <assert.h>
#include <iostream>
#include <stdexcept>

#include <SDL.h>


RoomBase::RoomBase()
	:
	Object("")
{
	fAreaOffset.x = fAreaOffset.y = 0;
}


RoomBase::~RoomBase()
{
}


/* virtual */
IE::rect
RoomBase::Frame() const
{
	return gfx_rect_to_rect(AreaRect());
}


GFX::rect
RoomBase::ViewPort() const
{
	return fScreenArea;
}


void
RoomBase::SetViewPort(GFX::rect rect)
{
	fScreenArea = rect;
}


IE::point
RoomBase::AreaOffset() const
{
	return fAreaOffset;
}


IE::rect
RoomBase::VisibleMapArea() const
{
	return fMapArea;
}


void
RoomBase::SetAreaOffset(IE::point point)
{
	GFX::rect areaRect = AreaRect();
	fAreaOffset = point;
	if (fAreaOffset.x < 0)
		fAreaOffset.x = 0;
	else if (fAreaOffset.x + fScreenArea.w > areaRect.w)
		fAreaOffset.x = std::max(areaRect.w - fScreenArea.w, 0);
	if (fAreaOffset.y < 0)
		fAreaOffset.y = 0;
	else if (fAreaOffset.y + fScreenArea.h > areaRect.h)
		fAreaOffset.y = std::max(areaRect.h - fScreenArea.h, 0);

	fMapArea = gfx_rect_to_rect(offset_rect_to(fScreenArea,
			fAreaOffset.x, fAreaOffset.y));
}


void
RoomBase::SetRelativeAreaOffset(int16 xDelta, int16 yDelta)
{
	IE::point newOffset = fAreaOffset;
	newOffset.x += xDelta;
	newOffset.y += yDelta;
	SetAreaOffset(newOffset);
}


void
RoomBase::CenterArea(const IE::point& point)
{
	IE::point destPoint;
	destPoint.x = point.x - fScreenArea.w / 2;
	destPoint.y = point.y - fScreenArea.y / 2;
	SetAreaOffset(destPoint);
}


void
RoomBase::ConvertToArea(GFX::rect& rect)
{
	rect.x += fAreaOffset.x;
	rect.y += fAreaOffset.y;
}


void
RoomBase::ConvertToArea(IE::point& point)
{
	point.x += fAreaOffset.x;
	point.y += fAreaOffset.y;
}


void
RoomBase::ConvertFromArea(GFX::rect& rect)
{
	rect.x -= fAreaOffset.x;
	rect.y -= fAreaOffset.y;
}


void
RoomBase::ConvertFromArea(IE::point& point)
{
	point.x -= fAreaOffset.x;
	point.y -= fAreaOffset.y;
}


/* static */
bool
RoomBase::IsPointPassable(const IE::point& point)
{
	/*uint8 state = RoomContainer::Get()->PointSearch(point);
	switch (state) {
		case 0:
		case 8:
		case 10:
		case 12:
		case 13:
			return false;
		default:
			return true;
	}*/
	return true;
}


void
RoomBase::ToggleOverlays()
{
}


void
RoomBase::TogglePolygons()
{
}


void
RoomBase::ToggleAnimations()
{
}


void
RoomBase::ToggleSearchMap()
{
}


void
RoomBase::HideGUI()
{
	// TODO
	ToggleGUI();
}


/* virtual */
void
RoomBase::ToggleDayNight()
{
}


/* virtual */
void
RoomBase::VideoAreaChanged(uint16 width, uint16 height)
{
	GFX::rect rect(0, 0, width, height);
	SetViewPort(rect);
}
