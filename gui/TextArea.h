/*
 * TextArea.h
 *
 *  Created on: 20/ott/2012
 *      Author: stefano
 */

#ifndef TEXTAREA_H_
#define TEXTAREA_H_

#include "Control.h"

#include <string>
#include <vector>

class Bitmap;
class TextArea: public Control {
public:
	class TextLine {
	public:
		TextLine();
		std::string text;
		uint16 width;
		uint16 height;
	};
	TextArea(IE::text_area* text);
	virtual ~TextArea();
	virtual void Draw();

	void AddText(const char* text);
	void SetText(const char* text);

private:
	//BAMResource* fInitialsFontResource;
	Bitmap* fBitmap;
	std::vector<TextLine*> fLines;
	bool fChanged;
};

#endif /* TEXTAREA_H_ */
