/*
 * BackWindow.h
 *
 *  Created on: 13/nov/2012
 *      Author: stefano
 */

#ifndef BACKWINDOW_H_
#define BACKWINDOW_H_

#include "Window.h"

const static uint16 kBackgroundWindowID = (uint16)-1;

class BackWindow: public Window {
public:
	BackWindow(const uint16 width, const uint16 height);
	~BackWindow();
};

#endif /* BACKWINDOW_H_ */
