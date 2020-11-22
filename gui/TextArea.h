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
class Font;
class Scrollbar;
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

	void SetScrollbar(Scrollbar* scrollbar);

	void AddText(const char* text);
	void ClearText();

	void ScrollBy(int16 x, int16 y);
private:
	//BAMResource* fInitialsFontResource;
	Bitmap* fBitmap;
	std::vector<TextLine*> fLines;
	int16 fYOffset;
	bool fChanged;

	Scrollbar* fScrollbar;

	void _UpdateScrollbar(int16 change);
};

#endif /* TEXTAREA_H_ */
