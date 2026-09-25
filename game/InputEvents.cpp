/*
 * InputEvents.cpp - see InputEvents.h
 */

#include "InputEvents.h"

#include "GUI.h"
#include "TextEdit.h"

bool
DispatchMouseEvent(GUI* gui, const SDL_Event& event)
{
	switch (event.type) {
		case SDL_MOUSEBUTTONDOWN:
			if (event.button.button == SDL_BUTTON_RIGHT)
				gui->RightMouseDown(event.button.x, event.button.y);
			else
				gui->MouseDown(event.button.x, event.button.y);
			return true;
		case SDL_MOUSEBUTTONUP:
			// A right click was delivered on the way down; no matching MouseUp
			// so a Button doesn't also fire its normal left-click action.
			if (event.button.button != SDL_BUTTON_RIGHT)
				gui->MouseUp(event.button.x, event.button.y);
			return true;
		case SDL_MOUSEMOTION:
			gui->MouseMoved(event.motion.x, event.motion.y);
			return true;
		default:
			return false;
	}
}


bool
DispatchKeyEvent(GUI* gui, const SDL_Event& event)
{
	TextEdit* edit = gui->TextFocus();
	if (edit == NULL)
		return false;

	switch (event.type) {
		case SDL_TEXTINPUT:
			edit->InsertText(event.text.text);
			return true;
		case SDL_KEYDOWN:
			if (event.key.keysym.sym == SDLK_BACKSPACE) {
				edit->Backspace();
				return true;
			}
			if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER) {
				edit->Enter();
				return true;
			}
			return false;
		default:
			return false;
	}
}
