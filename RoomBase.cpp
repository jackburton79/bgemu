#include "RoomBase.h"

#include "Bitmap.h"
#include "Graphics.h"
#include "GraphicsEngine.h"
#include "GUI.h"
#include "RectUtils.h"
#include "Timer.h"

#include <algorithm>
#include <assert.h>
#include <iostream>
#include <stdexcept>

#include <SDL.h>


RoomBase::RoomBase()
	:
	Object("RoomBase"),
	fLastScrollTime(0)
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
	return fViewPort;
}


void
RoomBase::SetViewPort(GFX::rect rect)
{
	fViewPort = rect;
}


IE::point
RoomBase::AreaOffset() const
{
	return fAreaOffset;
}


IE::point
RoomBase::AreaCenterPoint() const
{
	IE::point point = fAreaOffset;
	point.x = point.x + fViewPort.w / 2;
	point.y = point.y + fViewPort.h / 2;
	return point;
}


IE::rect
RoomBase::VisibleMapArea() const
{
	return fMapArea;
}


void
RoomBase::SetAreaOffset(const IE::point& point)
{
	GFX::rect areaRect = AreaRect();
	fAreaOffset = point;
	if (fAreaOffset.x < 0)
		fAreaOffset.x = 0;
	else if (fAreaOffset.x + fViewPort.w > areaRect.w)
		fAreaOffset.x = std::max(areaRect.w - fViewPort.w, 0);
	if (fAreaOffset.y < 0)
		fAreaOffset.y = 0;
	else if (fAreaOffset.y + fViewPort.h > areaRect.h)
		fAreaOffset.y = std::max(areaRect.h - fViewPort.h, 0);

	fMapArea = gfx_rect_to_rect(offset_rect_to(fViewPort,
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
RoomBase::SetAreaOffsetCenter(const IE::point& point)
{
	IE::point destPoint = LeftToppedOffset(point);
	SetAreaOffset(destPoint);
}


IE::point
RoomBase::CenteredOffset(const IE::point& point) const
{
	IE::point result = point;	
	result.x = result.x + fViewPort.w / 2;
	result.y = result.y + fViewPort.h / 2;
	SanitizeOffsetCenter(result);
	return result;
}


IE::point
RoomBase::LeftToppedOffset(const IE::point& point) const
{
	IE::point result = point;	
	result.x = result.x - fViewPort.w / 2;
	result.y = result.y - fViewPort.h / 2;
	SanitizeOffsetLeftTop(result);
	return result;
}


void
RoomBase::SanitizeOffsetLeftTop(IE::point& point) const
{
	const GFX::rect areaRect = AreaRect();
	point.x = std::max(0, (int)point.x);
	point.y = std::max(0, (int)point.y);
	point.x = std::min((int)point.x, areaRect.w - fViewPort.w);
	point.y = std::min((int)point.y, areaRect.h - fViewPort.h);
}


void
RoomBase::SanitizeOffsetCenter(IE::point& point) const
{
	const GFX::rect areaRect = AreaRect();
	point.x = std::max(fViewPort.w / 2, (int)point.x);
	point.y = std::max(fViewPort.h / 2, (int)point.y);
	point.x = std::min(areaRect.w - fViewPort.w / 2, (int)point.x);
	point.y = std::min(areaRect.h - fViewPort.h / 2, (int)point.y);
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


void
RoomBase::UpdateCursorAndScrolling(int x, int y, int scrollByX, int scrollByY)
{
	if (scrollByX == 0 && scrollByY == 0) {
		// TODO: Handle other cursors
		GUI::Get()->SetArrowCursor(IE::CURSOR_HAND);
		return;
	}

	int cursorIndex = 0;
	if (scrollByX > 0) {
		if (scrollByY > 0)
			cursorIndex = IE::CURSOR_ARROW_SE;
		else if (scrollByY < 0)
			cursorIndex = IE::CURSOR_ARROW_NE;
		else
			cursorIndex = IE::CURSOR_ARROW_E;
	} else if (scrollByX < 0) {
		if (scrollByY > 0)
			cursorIndex = IE::CURSOR_ARROW_SW;
		else if (scrollByY < 0)
			cursorIndex = IE::CURSOR_ARROW_NW;
		else
			cursorIndex = IE::CURSOR_ARROW_W;
	} else {
		if (scrollByY > 0)
			cursorIndex = IE::CURSOR_ARROW_S;
		else if (scrollByY < 0)
			cursorIndex = IE::CURSOR_ARROW_N;
		else
			cursorIndex = IE::CURSOR_ARROW_E;
	}

	GUI::Get()->SetArrowCursor(cursorIndex);

	const uint32 kScrollDelay = 100;
	
	uint32 ticks = Timer::Ticks();
	if (fLastScrollTime + kScrollDelay < ticks) {
		SetRelativeAreaOffset(scrollByX, scrollByY);
		fLastScrollTime = ticks;
	}
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
RoomBase::ToggleGUI()
{
	if (IsGUIShown())
		HideGUI();
	else
		ShowGUI();
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
