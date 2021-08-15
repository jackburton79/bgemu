/*
 * TextArea.cpp
 *
 *  Created on: 20/ott/2012
 *      Author: stefano
 */

#include "TextArea.h"

#include "BamResource.h"
#include "Bitmap.h"
#include "Core.h"
#include "Dialog.h"
#include "Game.h"
#include "GraphicsEngine.h"
#include "RectUtils.h"
#include "ResManager.h"
#include "Scrollbar.h"
#include "TextSupport.h"
#include "TLKResource.h"
#include "Utils.h"
#include "Window.h"


const static int16 kLineSpacing = 5;

TextArea::TextLine::TextLine()
	:
	width(0),
	height(0),
	dialog_option(-1)
{
}


IE::rect
TextArea::TextLine::Frame() const
{
	IE::rect frame = { 0, 0, (int16)width, (int16)height };
	return frame;
}


TextArea::TextArea(IE::text_area* text)
	:
	Control(text),
	fBitmap(NULL),
	fSelected(NULL),
	fYOffset(0),
	fChanged(true),
	fScrollbar(NULL)
{
	fBitmap = new Bitmap(text->w, text->h, 8);
/*
	GFX::Color foreground = { text->color1_r, text->color1_g, text->color1_b, text->color1_a };
	//GFX::Color transparent = { text->color2_r, text->color2_g, text->color2_b, text->color2_a };
	GFX::Color background = { text->color3_r, text->color3_g, text->color3_b, text->color3_a };
	GFX::Palette palette(foreground, background);
	*/
	GFX::Palette palette(*GFX::kPaletteYellow);
	GFX::Color transparent = palette.colors[0];
	fBitmap->SetPalette(palette);
	fBitmap->SetColorKey(transparent.r, transparent.g, transparent.b, true);
}


TextArea::~TextArea()
{
	if (fBitmap != NULL)
		fBitmap->Release();
}


/* virtual */
void
TextArea::Draw()
{
	GFX::rect destRect(fControl->x, fControl->y,
					   fControl->w, fControl->h);
	fWindow->ConvertToScreen(destRect);
	if (fChanged) {
		GFX::rect rect(0, -fYOffset, fBitmap->Width(), fBitmap->Height());
		std::string fontName = ((IE::text_area*)fControl)->font_bam.CString();
		fBitmap->Clear(0);
		uint32 flags = IE::LABEL_JUSTIFY_LEFT | IE::LABEL_JUSTIFY_BOTTOM;
		TextLines::const_iterator i;
		for (i = fLines.begin(); i != fLines.end(); i++) {
			int attr = 0;
			const TextLine& line = *i;
			GFX::point where = { rect.x, rect.y };
			if (&line == fSelected) {
				attr |= TEXT_SELECTED;
			}
			FontRoster::GetFont(fontName)->RenderString(line.text, flags | attr, fBitmap, false, where);
			rect.y += line.height + kLineSpacing;
		}
		fChanged = false;
	}
	GraphicsEngine::Get()->BlitToScreen(fBitmap, NULL, &destRect);
}


/* virtual */
void
TextArea::MouseDown(IE::point point)
{
	DialogHandler* dialog = Game::Get()->Dialog();
	if (dialog != NULL) {
		if (dialog->IsWaitingUserChoice()) {
			if (fSelected != NULL) {
				const int32 selectedIndex = fSelected->dialog_option - 1;
				// Deselect, then remove all dialog lines
				fSelected = NULL;
				for (;;) {
					TextLine& line = fLines.back();
					if (line.dialog_option == -1)
						break;
					fLines.pop_back();
				}
				dialog->SelectOption(selectedIndex);
				dialog->Continue();
			}
		} else
			dialog->Continue();
	}
}


/* virtual */
void
TextArea::MouseMoved(IE::point point, uint32 transit)
{
	Control::MouseMoved(point, transit);
	TextLine* oldSelected = fSelected;
	if (Game::Get()->Dialog() != NULL)
		fSelected = const_cast<TextLine*>(_HitTestLines(point));
	else
		fSelected = NULL;

	if (oldSelected != fSelected)
		fChanged = true;
}


void
TextArea::SetScrollbar(Scrollbar* scrollbar)
{
	fScrollbar = scrollbar;
}


void
TextArea::AddText(const char* text)
{
	std::string textString(text);
	_AddText(textString, -1);
}


void
TextArea::AddDialogText(const char* text, int32 dialogOption)
{
	_AddText(text, dialogOption);
}


void
TextArea::ClearText()
{
	fLines.clear();
	fChanged = true;
}


void
TextArea::SetLines(const TextLines& lines)
{
	fLines.clear();
	TextLines::const_iterator i;
	for (i = lines.begin(); i != lines.end(); i++)
		fLines.push_back(*i);
	fChanged = true;
}


void
TextArea::GetLines(TextLines& lines) const
{
	lines.clear();
	TextLines::const_iterator i;
	for (i = fLines.begin(); i != fLines.end(); i++)
		lines.push_back(*i);
}


void
TextArea::ScrollBy(int16 /* not implemented */, int16 y)
{
	fYOffset += y;
	if (fYOffset < 0)
		fYOffset = 0;
	fChanged = true;

	_UpdateScrollbar(y);
}


void
TextArea::ScrollTo(int16 /* not implemented */, int16 y)
{
	fYOffset = y;
	if (fYOffset < 0)
		fYOffset = 0;
	fChanged = true;

	_UpdateScrollbar(y);
}


void
TextArea::_AddText(std::string textString, int32 dialogOption)
{
	std::string fontName = ((IE::text_area*)fControl)->font_bam.CString();
	const Font* font = FontRoster::GetFont(fontName);

	while (!textString.empty()) {
		TextLine newLine;
		std::string textLine = font->TruncateString(textString, fControl->w, &newLine.width);
		newLine.text = textLine;
		newLine.height = font->Height();
		newLine.dialog_option = dialogOption;
		fLines.push_back(newLine);
	}

	TextLine& lastLine = fLines.back();
	int16 scrollToOffset = _LineOffset(&lastLine);
	scrollToOffset -= Height();
	ScrollTo(0, scrollToOffset);

	fChanged = true;
}


void
TextArea::_UpdateScrollbar(int16 change)
{
	if (fScrollbar != NULL)
		fScrollbar->UpdateOffset(fYOffset);
}


const TextArea::TextLine*
TextArea::_HitTestLines(IE::point point) const
{
	IE::point lineOffset = {
			(int16)fControl->x,
			(int16)(fControl->y - fYOffset)
	};
	TextLines::const_iterator i;
	for (i = fLines.begin(); i != fLines.end(); i++) {
		const TextLine& line = *i;
		const IE::rect frame = offset_rect(line.Frame(),
										   lineOffset.x, lineOffset.y);
		lineOffset.y += line.height + kLineSpacing;
		// skip non-dialog lines
		if (line.dialog_option == -1)
			continue;
		if (rect_contains(frame, point))
			return &line;
	}
	return NULL;
}


int16
TextArea::_LineOffset(TextLine* line) const
{
	int16 lineOffset = 0;
	TextLines::const_iterator i;
	for (i = fLines.begin(); i != fLines.end(); i++) {
		const TextLine& l = *i;
		lineOffset += l.height + kLineSpacing;
		if (&l == line)
			break;
	}
	return lineOffset;
}
