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
#include "Room.h"
#include "Window.h"

#include <assert.h>


Button::Button(IE::button* button)
	:
	Control(button),
	fResource(NULL),
	fEnabled(true),
	fSelected(false),
	fPressed(false)
{
	fResource = gResManager->GetBAM(button->image);
}


Button::~Button()
{
	gResManager->ReleaseResource(fResource);
}


/* virtual */
void
Button::AttachedToWindow(Window* window)
{
	Control::AttachedToWindow(window);
}


/* virtual */
void
Button::Draw()
{
	if (fResource == NULL)
		return;
	try {
		IE::button* button = (IE::button*)fControl;
		const Bitmap* frame;
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

		if (frame != NULL) {
			GFX::rect destRect(sint16(fControl->x), sint16(fControl->y),
					uint16(fControl->w), uint16(fControl->h));
			fWindow->ConvertToScreen(destRect);
			GraphicsEngine::Get()->BlitToScreen(frame, NULL, &destRect);
			//GraphicsEngine::DeleteBitmap(frame);
		}
	} catch (...) {

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
