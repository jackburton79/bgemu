/*
 * Scrollbar.cpp
 *
 *  Created on: 21/ott/2012
 *      Author: stefano
 */


#include "BamResource.h"
#include "GraphicsEngine.h"
#include "ResManager.h"
#include "Scrollbar.h"
#include "TextArea.h"
#include "Window.h"

#include <algorithm>


// Pixels scrolled per arrow click / per Pulse() while an arrow is held.
static const int32 kArrowStep = 16;


Scrollbar::Scrollbar(IE::scrollbar* scrollbar)
	:
	Control(scrollbar),
	fResource(NULL),
	fUpArrow(NULL),
	fUpArrowPressed(NULL),
	fDownArrow(NULL),
	fDownArrowPressed(NULL),
	fTrough(NULL),
	fThumb(NULL),
	fUpPressed(false),
	fDownPressed(false),
	fDraggingThumb(false),
	fThumbGrabOffset(0),
	fOffset(0),
	fRange(0)
{
	fResource = gResManager->GetBAM(scrollbar->bam);
	if (fResource == NULL)
		return;
	uint16 cycle = scrollbar->cycle;
	fUpArrow = fResource->FrameForCycle(cycle, scrollbar->arrow_up_unpressed);
	fUpArrowPressed = fResource->FrameForCycle(cycle, scrollbar->arrow_up_pressed);
	fDownArrow = fResource->FrameForCycle(cycle, scrollbar->arrow_down_unpressed);
	fDownArrowPressed = fResource->FrameForCycle(cycle, scrollbar->arrow_down_pressed);
	fTrough = fResource->FrameForCycle(cycle, scrollbar->trough);
	fThumb = fResource->FrameForCycle(cycle, scrollbar->slider);
}


Scrollbar::~Scrollbar()
{
	for (Bitmap* bitmap : { fUpArrow, fUpArrowPressed, fDownArrow,
						fDownArrowPressed, fTrough, fThumb }) {
		if (bitmap != NULL)
			bitmap->Release();
	}
	gResManager->ReleaseResource(fResource);
}


/* virtual */
void
Scrollbar::AttachedToWindow(::Window* window)
{
	Control::AttachedToWindow(window);
	uint16 textAreaID = ((IE::scrollbar*)fControl)->text_area_id;
	TextArea* textArea = dynamic_cast<TextArea*>(window->GetControlByID(textAreaID));
	if (textArea != NULL)
		textArea->SetScrollbar(this);
}


TextArea*
Scrollbar::_TextArea() const
{
	if (Window() == NULL)
		return NULL;
	uint16 textAreaID = ((IE::scrollbar*)fControl)->text_area_id;
	return dynamic_cast<TextArea*>(Window()->GetControlByID(textAreaID));
}


GFX::rect
Scrollbar::_UpArrowRect() const
{
	uint16 w = fUpArrow != NULL ? fUpArrow->Width() : 0;
	uint16 h = fUpArrow != NULL ? fUpArrow->Height() : 0;
	return GFX::rect(fControl->x + (fControl->w - w) / 2, fControl->y, w, h);
}


GFX::rect
Scrollbar::_DownArrowRect() const
{
	uint16 w = fDownArrow != NULL ? fDownArrow->Width() : 0;
	uint16 h = fDownArrow != NULL ? fDownArrow->Height() : 0;
	return GFX::rect(fControl->x + (fControl->w - w) / 2,
		fControl->y + fControl->h - h, w, h);
}


GFX::rect
Scrollbar::_TroughRect() const
{
	int16 top = fControl->y + (fUpArrow != NULL ? fUpArrow->Height() : 0);
	int16 bottom = fControl->y + fControl->h
		- (fDownArrow != NULL ? fDownArrow->Height() : 0);
	int16 height = bottom > top ? bottom - top : 0;
	return GFX::rect(fControl->x, top, fControl->w, height);
}


GFX::rect
Scrollbar::_ThumbRect() const
{
	GFX::rect trough = _TroughRect();
	uint16 thumbW = fThumb != NULL ? fThumb->Width() : 0;
	uint16 thumbH = fThumb != NULL ? fThumb->Height() : 0;
	int16 travel = (int16)trough.h - (int16)thumbH;
	if (travel < 0)
		travel = 0;
	int16 y = trough.y;
	if (fRange > 0)
		y += (int16)((int64)fOffset * travel / fRange);
	return GFX::rect(fControl->x + (fControl->w - thumbW) / 2, y, thumbW, thumbH);
}


/* virtual */
void
Scrollbar::Draw()
{
	if (fResource == NULL)
		return;

	// Trough: tile the (short) trough frame down its area.
	if (fTrough != NULL) {
		GFX::rect trough = _TroughRect();
		fWindow->ConvertToScreen(trough);
		GFX::rect clip = trough;
		GraphicsEngine::Get()->SetClipping(&clip);
		int16 x = trough.x + (trough.w - fTrough->Width()) / 2;
		for (int16 y = trough.y; y < trough.y + trough.h; y += fTrough->Height()) {
			GFX::rect dst(x, y, fTrough->Width(), fTrough->Height());
			GraphicsEngine::Get()->BlitToScreen(fTrough, NULL, &dst);
		}
		GraphicsEngine::Get()->SetClipping(NULL);
	}

	// Thumb: only when there's something to scroll.
	if (fThumb != NULL && fRange > 0) {
		GFX::rect thumb = _ThumbRect();
		fWindow->ConvertToScreen(thumb);
		GFX::rect dst(thumb.x, thumb.y, fThumb->Width(), fThumb->Height());
		GraphicsEngine::Get()->BlitToScreen(fThumb, NULL, &dst);
	}

	Bitmap* up = (fUpPressed && fUpArrowPressed != NULL) ? fUpArrowPressed : fUpArrow;
	if (up != NULL) {
		GFX::rect r = _UpArrowRect();
		fWindow->ConvertToScreen(r);
		GFX::rect dst(r.x, r.y, up->Width(), up->Height());
		GraphicsEngine::Get()->BlitToScreen(up, NULL, &dst);
	}

	Bitmap* down = (fDownPressed && fDownArrowPressed != NULL) ? fDownArrowPressed : fDownArrow;
	if (down != NULL) {
		GFX::rect r = _DownArrowRect();
		fWindow->ConvertToScreen(r);
		GFX::rect dst(r.x, r.y, down->Width(), down->Height());
		GraphicsEngine::Get()->BlitToScreen(down, NULL, &dst);
	}
}


/* virtual */
void
Scrollbar::MouseDown(IE::point point)
{
	if (_UpArrowRect().Contains(point.x, point.y)) {
		fUpPressed = true;
		_ScrollBy(-kArrowStep);
		return;
	}
	if (_DownArrowRect().Contains(point.x, point.y)) {
		fDownPressed = true;
		_ScrollBy(kArrowStep);
		return;
	}

	GFX::rect thumb = _ThumbRect();
	if (fRange > 0 && thumb.Contains(point.x, point.y)) {
		fDraggingThumb = true;
		fThumbGrabOffset = point.y - thumb.y;
		fWindow->SetMouseCapture(this);
		return;
	}

	// A click in the trough, above or below the thumb, pages by a
	// viewport height.
	GFX::rect trough = _TroughRect();
	if (trough.Contains(point.x, point.y)) {
		TextArea* textArea = _TextArea();
		int32 page = textArea != NULL ? textArea->Height() : 40;
		_ScrollBy(point.y < thumb.y ? -page : page);
	}
}


/* virtual */
void
Scrollbar::MouseMoved(IE::point point, uint32 /* transit */)
{
	if (!fDraggingThumb)
		return;

	GFX::rect trough = _TroughRect();
	int16 travel = (int16)trough.h - (fThumb != NULL ? (int16)fThumb->Height() : 0);
	if (travel <= 0)
		return;

	int32 relative = (point.y - fThumbGrabOffset) - trough.y;
	relative = std::max<int32>(0, std::min<int32>(relative, travel));
	_ScrollTo((int32)((int64)relative * fRange / travel));
}


/* virtual */
void
Scrollbar::MouseUp(IE::point /* point */)
{
	fUpPressed = false;
	fDownPressed = false;
	fDraggingThumb = false;
	if (fWindow != NULL)
		fWindow->SetMouseCapture(NULL);
}


/* virtual */
void
Scrollbar::Pulse()
{
	if (fUpPressed)
		_ScrollBy(-kArrowStep);
	else if (fDownPressed)
		_ScrollBy(kArrowStep);
}


void
Scrollbar::SetScrollInfo(int32 offset, int32 range)
{
	fOffset = offset;
	fRange = range > 0 ? range : 0;
}


void
Scrollbar::_ScrollTo(int32 offset)
{
	if (TextArea* textArea = _TextArea())
		textArea->ScrollTo(0, (int16)offset);
}


void
Scrollbar::_ScrollBy(int32 delta)
{
	if (TextArea* textArea = _TextArea())
		textArea->ScrollBy(0, (int16)delta);
}
