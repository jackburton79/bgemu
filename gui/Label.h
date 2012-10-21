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
class BAMResource;
class Label: public Control {
public:
	Label(IE::label* label);
	virtual ~Label();

	virtual void Draw();

private:
	BAMResource* fFontResource;
	Bitmap* fBitmap;
	Color* fColors;

	void _GeneratePalette(const Color& colorStart,
			const Color& colorEnd);
};

#endif /* __LABEL_H_ */
