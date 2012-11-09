/*
 * TextArea.h
 *
 *  Created on: 20/ott/2012
 *      Author: stefano
 */

#ifndef TEXTAREA_H_
#define TEXTAREA_H_

#include "Control.h"

class Bitmap;
class BAMResource;
class TextArea: public Control {
public:
	TextArea(IE::text_area* text);
	virtual ~TextArea();
	virtual void Draw();
	virtual void MouseDown(IE::point point);

private:
	BAMResource* fFontResource;
	BAMResource* fInitialsFontResource;
	Bitmap* fBitmap;
};

#endif /* TEXTAREA_H_ */
