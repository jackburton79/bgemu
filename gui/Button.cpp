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
#include "Window.h"

#include <assert.h>

Button::Button(IE::button* button)
	:
	Control(button),
	fEnabled(true),
	fSelected(false),
	fPressed(false)
{
	button->Print();
	fResource = gResManager->GetBAM(button->image);
}


Button::~Button()
{
	gResManager->ReleaseResource(fResource);
}


/* virtual */
void
Button::Draw()
{
	IE::button* button = (IE::button*)fControl;
	Frame frame;
	// TODO: Seems enabled and selected aren't used
	if (!fEnabled)
		frame = fResource->FrameForCycle(button->cycle, button->frame_disabled);
	else if (fPressed)
		frame = fResource->FrameForCycle(button->cycle, button->frame_pressed);
	/*else if (fSelected)
		frame = fResource->FrameForCycle(button->cycle, button->frame_selected);
	*/
	else
		frame = fResource->FrameForCycle(button->cycle, button->frame_unpressed);

	if (frame.bitmap != NULL) {
		GFX::rect destRect = { fControl->x, fControl->y, fControl->w, fControl->h };
		fWindow->ConvertToScreen(destRect);
		GraphicsEngine::Get()->BlitToScreen(frame.bitmap, NULL, &destRect);
		GraphicsEngine::DeleteBitmap(frame.bitmap);
	}
}


/* virtual */
void
Button::MouseMoved(IE::point point, uint32 transit)
{
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
	fPressed = true;
}


/* virtual */
void
Button::MouseUp(IE::point point)
{
	fPressed = false;
}
