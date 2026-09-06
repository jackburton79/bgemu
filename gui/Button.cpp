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
		// Center the icon over the button's own frame (an item icon BAM
		// isn't necessarily the same size as a 42x42 inventory slot).
		// Not GFX::rect::CenterIn() - despite the name it centers this
		// rect's *position* (x/y) within the argument, not this rect
		// itself as a same-sized box inside it; it has no other caller
		// in the codebase to have caught that.
		GFX::rect buttonFrame = Frame();
		int32 offsetX = ((int32)buttonFrame.w - (int32)fIcon->Width()) / 2;
		int32 offsetY = ((int32)buttonFrame.h - (int32)fIcon->Height()) / 2;
		GFX::rect iconRect(buttonFrame.x + offsetX, buttonFrame.y + offsetY,
			fIcon->Width(), fIcon->Height());
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
