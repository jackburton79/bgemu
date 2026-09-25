/*
 * TextEdit.cpp
 *
 *  Created on: 11/nov/2012
 *      Author: stefano
 */

#include "TextEdit.h"

#include "Bitmap.h"
#include "GraphicsEngine.h"
#include "GUI.h"
#include "TextSupport.h"
#include "Timer.h"
#include "Window.h"


// The caret shows for half of every second.
static const uint32 kCaretPeriodMs = 500;
// The font of a field whose record names none (BG1's name field), and the length of
// one without a length of its own.
static const char* const kDefaultFont = "NORMAL";
// A text field with no length of its own takes this many characters.
static const size_t kDefaultMaxLength = 32;


TextEdit::TextEdit(IE::text_edit* textEdit)
	:
	Control(textEdit),
	// 8 bit, like the labels with their own ink: the text is white whatever the
	// font's own colors are.
	fBitmap(new Bitmap(textEdit->w, textEdit->h, 8)),
	fMaxLength(textEdit->max_length != 0 ? textEdit->max_length : kDefaultMaxLength),
	fCaretShown(false)
{
	// Index 0 is the background, whatever the palette says.
	fBitmap->SetColorKey(0);
	fText = textEdit->initial_text;
	_Render(false);
}


TextEdit::~TextEdit()
{
	GUI* gui = GUI::Get();
	if (gui != nullptr && gui->TextFocus() == this)
		gui->SetTextFocus(nullptr);
	if (fBitmap != nullptr)
		fBitmap->Release();
}


/* virtual */
void
TextEdit::Draw()
{
	const bool focused = GUI::Get()->TextFocus() == this;
	const bool caret = focused && (Timer::Ticks() / kCaretPeriodMs) % 2 == 0;
	if (caret != fCaretShown)
		_Render(caret);

	GFX::rect destRect = Frame();
	fWindow->ConvertToScreen(destRect);
	GraphicsEngine::Get()->BlitToScreen(fBitmap, nullptr, &destRect);
}


/* virtual */
void
TextEdit::DetachedFromWindow(::Window* window)
{
	if (GUI::Get()->TextFocus() == this)
		GUI::Get()->SetTextFocus(nullptr);
	Control::DetachedFromWindow(window);
}


// A click gives the field the focus.
/* virtual */
void
TextEdit::MouseUp(IE::point point)
{
	GUI::Get()->SetTextFocus(this);
	Control::MouseUp(point);
}


void
TextEdit::SetText(const std::string& text)
{
	fText = text.substr(0, fMaxLength);
	_Render(fCaretShown);
	if (fOnChange)
		fOnChange();
}


void
TextEdit::InsertText(const std::string& text)
{
	const size_t room = fMaxLength > fText.size() ? fMaxLength - fText.size() : 0;
	if (room == 0 || text.empty())
		return;
	fText += text.substr(0, room);
	_Render(fCaretShown);
	if (fOnChange)
		fOnChange();
}


void
TextEdit::Backspace()
{
	if (fText.empty())
		return;
	// A UTF-8 character takes back all of its bytes.
	size_t last = fText.size() - 1;
	while (last > 0 && (static_cast<unsigned char>(fText[last]) & 0xc0) == 0x80)
		last--;
	fText.erase(last);
	_Render(fCaretShown);
	if (fOnChange)
		fOnChange();
}


void
TextEdit::Enter()
{
	if (fOnEnter)
		fOnEnter();
}


void
TextEdit::SetChangeCallback(std::function<void()> callback)
{
	fOnChange = std::move(callback);
}


void
TextEdit::SetEnterCallback(std::function<void()> callback)
{
	fOnEnter = std::move(callback);
}


// The text drawn on the bitmap, with an underscore for the caret.
void
TextEdit::_Render(bool caret)
{
	fCaretShown = caret;
	fBitmap->Clear(0);
	const std::string shown = caret ? fText + "_" : fText;
	if (!shown.empty()) {
		IE::text_edit* edit = static_cast<IE::text_edit*>(fControl);
		const std::string font = edit->font_bam.CString()[0] != '\0'
			? edit->font_bam.CString() : kDefaultFont;
		const GFX::Palette ink(GFX::Color{ 255, 255, 255, 0 }, GFX::Color{ 0, 0, 0, 0 });
		FontRoster::GetFont(font)->RenderString(shown, 0, fBitmap, &ink);
	}
}
