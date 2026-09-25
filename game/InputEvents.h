/*
 * InputEvents.h
 *
 * The mouse events every screen of the game takes the same way, whichever loop
 * (the game's, the front end's) polled them.
 */

#pragma once

#include <SDL.h>

class GUI;

// Hands a mouse event (a button pressed or released, the pointer moved) to the
// GUI; a right button goes down as a right click and its release is dropped, so
// a button doesn't also fire its left click. Returns false for any other event.
bool DispatchMouseEvent(GUI* gui, const SDL_Event& event);
