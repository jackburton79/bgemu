/*
 * Label.h
 *
 *  Created on: 20/ott/2012
 *      Author: stefano
 */

#ifndef __LABEL_H_
#define __LABEL_H_

#include "Control.h"

struct Color;
class Bitmap;
class Label: public Control {
public:
	Label(IE::label* label);
	virtual ~Label();

	void SetText(const std::string& text);
	virtual void Draw();

private:
	Bitmap* fBitmap;
};

#endif /* __LABEL_H_ */
