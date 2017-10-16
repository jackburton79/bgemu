/*
 * BackWindow.cpp
 *
 *  Created on: 13/nov/2012
 *      Author: stefano
 */

#include "BackWindow.h"
#include "Control.h"

BackWindow::BackWindow(const uint16 width, const uint16 height)
	:
	Window(kBackgroundWindowID, 0, 0, width, height, NULL)
{
	IE::control* fakeControl = new IE::control;
	fakeControl->x = 0;
	fakeControl->y = 0;
	fakeControl->w = Width();
	fakeControl->h = Height();
	fakeControl->type = 0;
	fakeControl->id = (uint32)-1;

	Add(new Control(fakeControl));
}


BackWindow::~BackWindow()
{

}

