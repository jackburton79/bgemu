/*
 * Button.cpp
 *
 *  Created on: 19/ott/2012
 *      Author: stefano
 */

#include "BamResource.h"
#include "Button.h"
#include "GraphicsEngine.h"
#include "ResManager.h"
#include "RoomBase.h"
#include "Window.h"


Button::Button(IE::button* button)
	:
	Control(button),
	fDisabledBitmap(NULL),
	fSelectedBitmap(NULL),
	fPressedBitmap(NULL),
	fUnpressedBitmap(NULL),
	fIcon(NULL),
	fEnabled(true),
	fSelected(false),
	fPressed(false)
{
	BAMResource *resource = gResManager->GetBAM(button->image);
	if (resource != NULL) {
		fDisabledBitmap = resource->FrameForCycle(button->cycle, button->frame_disabled);
		fSelectedBitmap = resource->FrameForCycle(button->cycle, button->frame_selected); 
		fPressedBitmap = resource->FrameForCycle(button->cycle, button->frame_pressed);
		fUnpressedBitmap = resource->FrameForCycle(button->cycle, button->frame_unpressed);
		gResManager->ReleaseResource(resource);
	}
}


Button::~Button()
{
	if (fDisabledBitmap != NULL)
		fDisabledBitmap->Release();
	if (fSelectedBitmap != NULL)
		fSelectedBitmap->Release();
	if (fPressedBitmap != NULL)
		fPressedBitmap->Release();
	if (fUnpressedBitmap != NULL)
		fUnpressedBitmap->Release();
	if (fIcon != NULL)
		fIcon->Release();
}


void
Button::SetIcon(Bitmap* icon)
{
	if (icon == fIcon)
		return;
	if (fIcon != NULL)
		fIcon->Release();
	fIcon = icon;
	if (fIcon != NULL) {
		// Precompute the icon's centered position within the button's
		// own (window-local) frame here rather than in Draw() - both
		// inputs (the icon's fixed size, the button's static CHU-
		// authored frame) are unchanged between frames, so redoing this
		// on every single Draw() call would be wasted work.
		fIconRect = GFX::rect(0, 0, fIcon->Width(), fIcon->Height());
		fIconRect.CenterIn(Frame());
	}
}


/* virtual */
void
Button::AttachedToWindow(::Window* window)
{
	Control::AttachedToWindow(window);
}


/* virtual */
void
Button::Draw()
{
	const Bitmap* frame;
	// TODO: Seems enabled and selected aren't used
	if (!fEnabled)
		frame = fDisabledBitmap;
	else if (fPressed)
		frame = fPressedBitmap;
	/*else if (fSelected)
		frame = fSelectedBitmap;
	*/
	else
		frame = fUnpressedBitmap;
	if (frame != NULL) {
		GFX::rect destRect = Frame();
		fWindow->ConvertToScreen(destRect);
		GraphicsEngine::Get()->BlitToScreen(frame, NULL, &destRect);
	}
	if (fIcon != NULL) {
		// fIconRect (an item icon centered over the button's own frame,
		// e.g. for an inventory slot) was already computed in SetIcon()
		// - only the screen conversion needs to happen every frame.
		GFX::rect iconRect = fIconRect;
		fWindow->ConvertToScreen(iconRect);
		GraphicsEngine::Get()->BlitToScreen(fIcon, NULL, &iconRect);
	}
	Control::Draw();
}


/* virtual */
void
Button::MouseMoved(IE::point point, uint32 transit)
{
	Control::MouseMoved(point, transit);

	if (transit == Control::MOUSE_ENTER)
		fSelected = true;
	else if (transit == Control::MOUSE_EXIT) {
		fSelected = false;
		fPressed = false;
	}
}


/* virtual */
void
Button::MouseDown(IE::point point)
{
	Control::MouseDown(point);
	fPressed = true;
}


/* virtual */
void
Button::MouseUp(IE::point point)
{
	Control::MouseUp(point);
	Invoke();
	fPressed = false;
}
