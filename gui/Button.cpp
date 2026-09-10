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
#include "TextSupport.h"
#include "Window.h"

#include <string>


Button::Button(IE::button* button)
	:
	Control(button),
	fDisabledBitmap(NULL),
	fSelectedBitmap(NULL),
	fPressedBitmap(NULL),
	fUnpressedBitmap(NULL),
	fIcon(NULL),
	fIconCount(0),
	fCoverBackground(false),
	fHighlighted(false),
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
Button::SetIconCount(int count)
{
	fIconCount = count;
}


void
Button::SetIcon(Bitmap* icon, bool coverBackground)
{
	fCoverBackground = (icon != NULL) && coverBackground;
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


void
Button::SetHighlighted(bool highlighted)
{
	fHighlighted = highlighted;
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
	if (frame != NULL && !fCoverBackground) {
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

		if (fIconCount > 1) {
			const Font* font = FontRoster::GetFont("TOOLFONT");
			if (font != NULL) {
				Bitmap* text = font->GetRenderedString(std::to_string(fIconCount), 0);
				if (text != NULL) {
					GFX::rect where(iconRect.x + iconRect.w - text->Width(),
									iconRect.y + iconRect.h - text->Height(),
									text->Width(), text->Height());
					GraphicsEngine::Get()->BlitToScreen(text, NULL, &where);
					text->Release();
				}
			}
		}
	}
	if (fHighlighted) {
		GFX::rect outline = Frame();
		fWindow->ConvertToScreen(outline);
		Bitmap* screen = GraphicsEngine::Get()->ScreenBitmap();
		uint32 color = screen->MapRGBColor(0, 255, 0);
		screen->StrokeRect(outline, color);
		outline.x += 1;
		outline.y += 1;
		outline.w -= 2;
		outline.h -= 2;
		screen->StrokeRect(outline, color);
	}
	Control::Draw();
}


/* virtual */
void
Button::MouseMoved(IE::point point, uint32 transit)
{
	Control::MouseMoved(point, transit);

	if (transit == Control::MOUSE_ENTER) {
		fSelected = true;
		NotifyHovered(true);
	} else if (transit == Control::MOUSE_EXIT) {
		fSelected = false;
		fPressed = false;
		NotifyHovered(false);
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
bool
Button::RightMouseDown(IE::point point)
{
	InvokeRightClick();
	return true;
}


/* virtual */
void
Button::MouseUp(IE::point point)
{
	Control::MouseUp(point);
	// fPressed cleared *before* Invoke(), not after: a control's action
	// can synchronously tear down and rebuild the whole GUI (e.g. a
	// Save/Load button whose click reloads the area, which reloads the
	// HUD CHU, which clears every window - this Button's own included);
	// touching `this` after that point would be a use-after-free.
	fPressed = false;
	Invoke();
}
