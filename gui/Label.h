/*
 * Label.h
 *
 *  Created on: 20/ott/2012
 *      Author: stefano
 */

#pragma once

#include "Control.h"

class Bitmap;
class Label: public Control {
public:
	Label(IE::label* label);
	virtual ~Label();

	void MouseMoved(IE::point point, uint32 transit) override;
	void SetText(const std::string& text);
	void Draw() override;

private:
	Bitmap* fBitmap;
};
