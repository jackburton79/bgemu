/*
 * Slider.h
 *
 *  Created on: 20/ott/2012
 *      Author: stefano
 */

#pragma once

#include "Control.h"

class BAMResource;
class Bitmap;
class Slider: public Control {
public:
	Slider(IE::slider* slider);
	virtual ~Slider();

	void Draw() override;
	void MouseDown(IE::point point) override;
	void MouseUp(IE::point point) override;
	void MouseMoved(IE::point point, uint32 transit) override;

private:
	Bitmap* fBackground;
	BAMResource* fKnobImage;
	int16 fKnobPosition;
};
