/*
 * Scrollbar.h
 *
 *  Created on: 21/ott/2012
 *      Author: stefano
 */

#ifndef SCROLLBAR_H_
#define SCROLLBAR_H_

#include "Control.h"

namespace GFX {
	struct rect;
}

class BAMResource;
class Scrollbar: public Control {
public:
	Scrollbar(IE::scrollbar* scrollbar);
	virtual ~Scrollbar();

	virtual void AttachedToWindow(::Window* window);
	virtual void Draw();
	virtual void MouseMoved(IE::point point, uint32 transit);
	virtual void MouseDown(IE::point point);
	virtual void MouseUp(IE::point point);
	virtual void Pulse();

private:
	BAMResource* fResource;
	Bitmap* fUpArrow;
	Bitmap* fDownArrow;
	bool fUpArrowPressed;
	bool fDownArrowPressed;

	void _DrawTrough(const GFX::rect& screenRect);
	void _DrawSlider(const GFX::rect& screenRect);
	void _DrawUpArrow(const GFX::rect& screenRect);
	void _DrawDownArrow(const GFX::rect& screenRect);
};

#endif /* SCROLLBAR_H_ */
