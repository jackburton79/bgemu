/*
 * Window.h
 *
 *  Created on: 18/ott/2012
 *      Author: stefano
 */

#ifndef __WINDOW_H_
#define __WINDOW_H_

#include <vector>

#include "IETypes.h"
#include "SupportDefs.h"

class Bitmap;
class CHUIResource;
class Control;
namespace GFX {
	struct rect;
}
class Window {
public:
	Window(uint16 id, int16 xPos, int16 yPos, int16 width, int16 height,
			Bitmap* background);
	~Window();

	void Draw();
	void Add(Control* control);

	uint16 ID() const;

	IE::point Position() const;
	uint16 Width() const;
	uint16 Height() const;
	void SetFrame(uint16 x, uint16 y, uint16 width, uint16 height);

	Control* GetControlByID(uint32 id) const;

	void MouseDown(IE::point point);
	void MouseUp(IE::point point);
	void MouseMoved(IE::point point);

	void ConvertToScreen(IE::point& point) const;
	void ConvertToScreen(GFX::rect& rect) const;

	void ConvertFromScreen(IE::point& point) const;

	void Print() const;
private:
	Control* _GetControl(IE::point point);

	uint16 fID;
	Bitmap* fBackground;
	std::vector<Control*> fControls;
	IE::point fPosition;
	uint16 fWidth;
	uint16 fHeight;
	Control* fActiveControl;
};


#endif /* WINDOW_H_ */
