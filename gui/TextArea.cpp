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
	fYOffset(0),
	fChanged(true),
	fScrollbar(NULL)
{
	fBitmap = new Bitmap(text->w, text->h, 8);

	GFX::Color foreground = { text->color1_r, text->color1_g, text->color1_b, text->color1_a };
	//GFX::Color transparent = { text->color2_r, text->color2_g, text->color2_b, text->color2_a };
	GFX::Color background = { text->color3_r, text->color3_g, text->color3_b, text->color3_a };

	GFX::Palette palette(foreground, background);
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
		std::vector<TextLine*>::const_iterator i;
		for (i = fLines.begin(); i != fLines.end(); i++) {
			const TextLine* line = *i;
			GFX::point where = { rect.x, rect.y };
			FontRoster::GetFont(fontName)->RenderString(line->text, flags, fBitmap, false, where);
			rect.y += line->height + kLineSpacing;
		}
		fChanged = false;
	}
	GraphicsEngine::Get()->BlitToScreen(fBitmap, NULL, &destRect);
}


/* virtual */
void
TextArea::MouseDown(IE::point point)
{
	DialogState* dialog = Game::Get()->Dialog();
	if (dialog != NULL) {
		const TextLine* line = _HitTestLine(point);
		if (line != NULL) {
			dialog->SelectOption(line->dialog_option);
		}
		Game::Get()->HandleDialog();
	}
}


/* virtual */
void
TextArea::MouseMoved(IE::point point, uint32 transit)
{
	Control::MouseMoved(point, transit);
}


void
TextArea::SetScrollbar(Scrollbar* scrollbar)
{
	fScrollbar = scrollbar;
}


void
TextArea::AddText(const char* text)
{
	std::string fontName = ((IE::text_area*)fControl)->font_bam.CString();
	const Font* font = FontRoster::GetFont(fontName);

	std::string textString(text);
	while (!textString.empty()) {
		std::string textLine = font->TruncateString(textString, fControl->w);
		TextLine* newLine = new TextLine();
		newLine->text = textLine;
		newLine->width = font->StringWidth(textLine, &newLine->height);
		fLines.push_back(newLine);
	}
	fChanged = true;
}


// TODO: dublicated code with the above function
void
TextArea::AddDialogText(const char* pre, const char* text, int32 dialogOption)
{
	std::string fontName = ((IE::text_area*)fControl)->font_bam.CString();
	const Font* font = FontRoster::GetFont(fontName);

	std::string textString(pre);
	textString.append(text);
	while (!textString.empty()) {
		std::string textLine = font->TruncateString(textString, fControl->w);
		TextLine* newLine = new TextLine();
		newLine->text = textLine;
		newLine->width = font->StringWidth(textLine, &newLine->height);
		newLine->dialog_option = dialogOption;
		fLines.push_back(newLine);
	}
	fChanged = true;
}


void
TextArea::ClearText()
{
	for (std::vector<TextLine*>::const_iterator i = fLines.begin();
			i != fLines.end(); i++) {
		delete *i;
	}
	fLines.clear();
	fChanged = true;
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
TextArea::_UpdateScrollbar(int16 change)
{
	if (fScrollbar != NULL)
		fScrollbar->UpdateOffset(fYOffset);
}


const TextArea::TextLine*
TextArea::_HitTestLine(IE::point point) const
{
	IE::point lineOffset = {(int16)fControl->x, (int16)(fControl->y + fYOffset)};
	std::vector<TextLine*>::const_iterator i;
	for (i = fLines.begin(); i != fLines.end(); i++) {
		const TextLine* line = *i;
		IE::rect frame = line->Frame();
		frame = offset_rect(frame, lineOffset.x, lineOffset.y);
		lineOffset.y += line->height + kLineSpacing;
		// skip non-dialog lines
		if (line->dialog_option == -1)
				continue;
		if (rect_contains(frame, point))
			return line;
	}
	return NULL;
}
