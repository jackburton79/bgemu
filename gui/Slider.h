/*
 * Slider.h
 *
 *  Created on: 20/ott/2012
 *      Author: stefano
 */

#ifndef SLIDER_H_
#define SLIDER_H_

#include "Control.h"

class BAMResource;
class Bitmap;
class Slider: public Control {
public:
	Slider(IE::slider* slider);
	virtual ~Slider();

	virtual void Draw();
	virtual void MouseDown(IE::point point);
	virtual void MouseUp(IE::point point);
	virtual void MouseMoved(IE::point point, uint32 transit);

private:
	Bitmap* fBackground;
	BAMResource* fKnobImage;
	int16 fKnobPosition;
};

#endif /* SLIDER_H_ */
