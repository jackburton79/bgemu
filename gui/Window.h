/*
 * Window.h
 *
 *  Created on: 18/ott/2012
 *      Author: stefano
 */

#ifndef __WINDOW_H_
#define __WINDOW_H_

#include <vector>

class Bitmap;
class CHUIResource;
class Control;
namespace GFX {
	struct rect;
}
class Window {
public:
	Window(int16 xPos, int16 yPos, int16 width, int16 height,
			Bitmap* background);
	~Window();
	void Draw();
	void Add(Control* control);

	IE::point Position() const;
	uint16 Width() const;
	uint16 Height() const;

	void MouseDown(IE::point point);
	void MouseUp(IE::point point);
	void MouseMoved(IE::point point);

	void ConvertToScreen(IE::point& point);
	//IE::point ConvertToScreen(const IE::point point);
	void ConvertToScreen(GFX::rect& rect);

	void Print() const;
private:
	Control* _GetControl(IE::point point);

	Bitmap* fBackground;
	std::vector<Control*> fControls;
	IE::point fPosition;
	uint16 fWidth;
	uint16 fHeight;
	Control* fActiveControl;
};


#endif /* WINDOW_H_ */
