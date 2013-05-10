/*
 * BackWindow.cpp
 *
 *  Created on: 13/nov/2012
 *      Author: stefano
 */

#include "BackWindow.h"
#include "Control.h"
#include "GraphicsEngine.h"

BackWindow::BackWindow()
	:
	Window(kBackgroundWindowID, 0, 0,
			GraphicsEngine::Get()->VideoArea().w,
			GraphicsEngine::Get()->VideoArea().h,
			NULL)

{
	//std::cout << "backwindow: " << Width() << " " << Height() << std::endl;
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

