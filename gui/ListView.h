/*
 * ListView.h
 *
 *  Created on: 13/set/2017
 *      Author: stefano
 */

#ifndef GUI_LISTVIEW_H_
#define GUI_LISTVIEW_H_

#include <string>

#include "Control.h"

class BAMResource;
class Bitmap;
class Window;
class ListView : public Control {
public:
	ListView(IE::control* control);
	virtual ~ListView();

	virtual void Draw();
	virtual void AttachedToWindow(::Window* window);
	virtual void MouseMoved(IE::point point, uint32 transit);
	virtual void MouseDown(IE::point point);
	virtual void MouseUp(IE::point point);

	void AddItem(std::string string);
private:
	Bitmap* fBitmap;
	StringList fList;
};

#endif /* GUI_LISTVIEW_H_ */
