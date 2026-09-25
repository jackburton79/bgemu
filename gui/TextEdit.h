/*
 * TextEdit.h
 *
 *  Created on: 11/nov/2012
 *      Author: stefano
 */

#pragma once

#include "Control.h"

#include <functional>
#include <string>

class Bitmap;

// A line of text the player types: what is typed comes through GUI::TextFocus()
// (InsertText(), Backspace(), Enter()), the text is drawn with the control's font
// and a blinking caret while it has the focus.
class TextEdit: public Control {
public:
	TextEdit(IE::text_edit* textEdit);
	virtual ~TextEdit();

	void Draw() override;
	void DetachedFromWindow(::Window* window) override;
	void MouseUp(IE::point point) override;

	void SetText(const std::string& text);
	const std::string& Text() const { return fText; }

	// Appends typed text (what does not fit the control's length is dropped).
	void InsertText(const std::string& text);
	void Backspace();
	// Return pressed.
	void Enter();

	// Called after every change of the text, and on Return.
	void SetChangeCallback(std::function<void()> callback);
	void SetEnterCallback(std::function<void()> callback);

private:
	void _Render(bool caret);

	Bitmap* fBitmap;
	std::string fText;
	size_t fMaxLength;
	bool fCaretShown;
	std::function<void()> fOnChange;
	std::function<void()> fOnEnter;
};
