/*
 * Button.h
 *
 *  Created on: 19/ott/2012
 *      Author: stefano
 */

#ifndef __BUTTON_H_
#define __BUTTON_H_

#include "Control.h"
#include "IETypes.h"

class BAMResource;
class Window;
class Button: public Control {
public:
	Button(IE::button* button);
	virtual ~Button();
	virtual void AttachedToWindow(::Window* window);
	virtual void Draw();
	virtual void MouseMoved(IE::point point, uint32 transit);
	virtual void MouseDown(IE::point point);
	virtual void MouseUp(IE::point point);

private:
	BAMResource* fResource;
	bool fEnabled;
	bool fSelected;
	bool fPressed;
};

#endif /* BUTTON_H_ */
