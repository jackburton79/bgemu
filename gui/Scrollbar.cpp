/*
 * Scrollbar.cpp
 *
 *  Created on: 21/ott/2012
 *      Author: stefano
 */


#include "BamResource.h"
#include "GraphicsEngine.h"
#include "RectUtils.h"
#include "ResManager.h"
#include "Scrollbar.h"
#include "Window.h"


Scrollbar::Scrollbar(IE::scrollbar* scrollbar)
	:
	Control(scrollbar),
	fResource(NULL),
	fUpArrowPressed(false),
	fDownArrowPressed(false)
{
	fResource = gResManager->GetBAM(scrollbar->bam);
	fUpArrow = fResource->FrameForCycle(scrollbar->cycle,
			scrollbar->arrow_up_unpressed);
	fDownArrow = fResource->FrameForCycle(scrollbar->cycle,
				scrollbar->arrow_down_unpressed);
}


Scrollbar::~Scrollbar()
{
	gResManager->ReleaseResource(fResource);
	GraphicsEngine::DeleteBitmap(fDownArrow);
	GraphicsEngine::DeleteBitmap(fUpArrow);
}


/* virtual */
void
Scrollbar::AttachedToWindow(Window* window)
{
	Control::AttachedToWindow(window);

	GFX::rect upRect = fUpArrow->Frame();
	GFX::rect downRect = fUpArrow->Frame();

	window->ConvertToScreen(upRect);
	window->ConvertToScreen(downRect);

	fUpArrow->SetPosition(upRect.x, upRect.y);
	fDownArrow->SetPosition(downRect.x, downRect.y);

}


/* virtual */
void
Scrollbar::Draw()
{
	GFX::rect destRect = { fControl->x, fControl->y, fControl->w, fControl->h };
	fWindow->ConvertToScreen(destRect);

	_DrawTrough(destRect);
	//_DrawSlider(destRect);
	_DrawUpArrow(destRect);
	_DrawDownArrow(destRect);
}


/* virtual */
void
Scrollbar::MouseDown(IE::point point)
{
	if (rect_contains(fUpArrow->Frame(), point)) {
		fUpArrowPressed = true;
	} else if (rect_contains(fDownArrow->Frame(), point)) {
		fDownArrowPressed = true;
	}
}


/* virtual */
void
Scrollbar::MouseUp(IE::point point)
{
	fUpArrowPressed = false;
	fDownArrowPressed = false;
}


/* virtual */
void
Scrollbar::MouseMoved(IE::point point, uint32 transit)
{
}


void
Scrollbar::_DrawTrough(const GFX::rect& screenRect)
{
	IE::scrollbar* scrollbar = (IE::scrollbar*)fControl;

	Bitmap* frame = fResource->FrameForCycle(scrollbar->cycle,
					scrollbar->trough);
	GFX::rect destRect = { screenRect.x, screenRect.y + 40,
			frame->Width(), frame->Height() };
	GraphicsEngine::Get()->BlitToScreen(frame, NULL, &destRect);
	GraphicsEngine::DeleteBitmap(frame);
}


void
Scrollbar::_DrawSlider(const GFX::rect& screenRect)
{
	IE::scrollbar* scrollbar = (IE::scrollbar*)fControl;

	Bitmap* frame = fResource->FrameForCycle(scrollbar->cycle,
					scrollbar->slider);
	GFX::rect destRect = { screenRect.x, screenRect.y + 20,
			frame->Width(), frame->Height() };
	GraphicsEngine::Get()->BlitToScreen(frame, NULL, &destRect);
	GraphicsEngine::DeleteBitmap(frame);
}


void
Scrollbar::_DrawUpArrow(const GFX::rect& screenRect)
{
	IE::scrollbar* scrollbar = (IE::scrollbar*)fControl;

	GraphicsEngine::DeleteBitmap(fUpArrow);
	fUpArrow = fResource->FrameForCycle(scrollbar->cycle,
		fUpArrowPressed ? scrollbar->arrow_up_pressed : scrollbar->arrow_up_unpressed);

	GFX::rect destRect = { screenRect.x, screenRect.y,
			fUpArrow->Width(), fUpArrow->Height() };
	fUpArrow->SetPosition(destRect.x, destRect.y);
	GraphicsEngine::Get()->BlitToScreen(fUpArrow, NULL, &destRect);
}


void
Scrollbar::_DrawDownArrow(const GFX::rect& screenRect)
{
	IE::scrollbar* scrollbar = (IE::scrollbar*)fControl;

	GraphicsEngine::DeleteBitmap(fDownArrow);
	fDownArrow = fResource->FrameForCycle(scrollbar->cycle,
		fDownArrowPressed ? scrollbar->arrow_down_pressed : scrollbar->arrow_down_unpressed);

	GFX::rect destRect = { screenRect.x,
			screenRect.y + fControl->h - fDownArrow->Height(),
			fDownArrow->Width(), fDownArrow->Height() };
	fDownArrow->SetPosition(destRect.x, destRect.y);
	GraphicsEngine::Get()->BlitToScreen(fDownArrow, NULL, &destRect);
}
