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
		IE::rect Frame() const;
		std::string text;
		uint16 width;
		uint16 height;
		int32 dialog_option;
	};
	TextArea(IE::text_area* text);
	virtual ~TextArea();
	virtual void Draw();
	virtual void MouseDown(IE::point point);
	virtual void MouseMoved(IE::point point, uint32 transit);

	void SetScrollbar(Scrollbar* scrollbar);

	void AddText(const char* text);
	void AddDialogText(const char*, const char* text, int32 dialogOption);
	void ClearText();
	
	typedef std::vector<TextArea::TextLine> TextLines;
	
	void SetLines(const TextLines& lines);
	void GetLines(TextLines& lines) const;

	void ScrollBy(int16 x, int16 y);
	void ScrollTo(int16 x, int16 y);

private:
	Bitmap* fBitmap;
	
	TextLines fLines;
	TextLine* fSelected;

	int16 fYOffset;
	bool fChanged;

	Scrollbar* fScrollbar;

	void _AddText(std::string textString, int32 dialogOption);
	void _UpdateScrollbar(int16 change);
	const TextLine* _HitTestLines(IE::point point) const;
	int16 _LineOffset(TextLine* line) const;
};

#endif /* TEXTAREA_H_ */
