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
#include "Scrollbar.h"
#include "TextSupport.h"
#include "Window.h"


const static int16 kLineSpacing = 5;


TextArea::TextLine::TextLine()
	:
	attributes(0),
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



// TextArea
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
#if 1
	GFX::Color foreground = { text->color1_r, text->color1_g, text->color1_b, text->color1_a };
	//GFX::Color transparent = { text->color2_r, text->color2_g, text->color2_b, text->color2_a };
	GFX::Color background = { text->color3_r, text->color3_g, text->color3_b, text->color3_a };
	GFX::Palette palette(foreground, background);
#else
	GFX::Palette palette(*GFX::kPaletteYellow);
#endif
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
		const Font* font = FontRoster::GetFont(fontName);
		fBitmap->Clear(0);
		uint32 flags = IE::LABEL_JUSTIFY_LEFT | IE::LABEL_JUSTIFY_BOTTOM;

		// A dialog option that word-wraps to more than one TextLine
		// (_AddText()'s own loop) gives every wrapped line the same
		// dialog_option - the whole run should highlight together, not
		// just whichever single line the mouse is hit-testing against
		// (fSelected itself, see MouseMoved()). Restricted to the
		// contiguous run fSelected itself sits in, not just any line
		// anywhere in the scrollback that happens to share the same
		// dialog_option value: that index is only meaningful within one
		// dialog turn (1, 2, 3, ...), and fLines keeps the whole
		// conversation's history - an earlier turn's own "option 1"
		// would otherwise light up too, just because both are numbered 1.
		size_t selectedStart = 0, selectedEnd = 0;
		if (fSelected != NULL) {
			size_t selectedIndex = (size_t)(fSelected - fLines.data());
			selectedStart = selectedEnd = selectedIndex;
			while (selectedStart > 0 && fLines[selectedStart - 1].dialog_option
					== fSelected->dialog_option)
				selectedStart--;
			while (selectedEnd + 1 < fLines.size() && fLines[selectedEnd + 1].dialog_option
					== fSelected->dialog_option)
				selectedEnd++;
		}

		for (size_t lineIndex = 0; lineIndex < fLines.size(); lineIndex++) {
			int attr = 0;
			const TextLine& line = fLines[lineIndex];
			GFX::point where = { rect.x, rect.y };
			if (fSelected != NULL && lineIndex >= selectedStart && lineIndex <= selectedEnd)
				attr |= TEXT_SELECTED;
			// TODO: Pass textarea palette
			// TODO: Should we apply palette to the bitmap ?
			// somehow it doesn't get the correct palette
			Bitmap* tmpBitmap = font->GetRenderedString(line.text, flags | attr, GFX::kPaletteRed);
			tmpBitmap->BlitTo(fBitmap, where);
			tmpBitmap->Release();
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
				while (!fLines.empty()) {
					TextLine& line = fLines.back();
					if (line.dialog_option == -1)
						break;

					fLines.pop_back();
				}
				dialog->SelectOption(selectedIndex);
				// SelectOption() can synchronously terminate the dialog
				// (e.g. a transition whose actions include
				// STARTCUTSCENE) - `dialog` may already be a dangling
				// pointer here. Game::InDialogMode() is safe to check
				// regardless (never touches `dialog` itself); calling
				// anything on `dialog` after that would not be.
				if (Game::Get()->InDialogMode() && !dialog->Continue())
					Game::Get()->TerminateDialog();
			}
		} else
			if (!dialog->Continue())
				Game::Get()->TerminateDialog();
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
	ScrollTo(0, fYOffset + y);
}


void
TextArea::ScrollTo(int16 /* not implemented */, int16 y)
{
	int16 maxOffset = _MaxYOffset();
	fYOffset = y < 0 ? 0 : (y > maxOffset ? maxOffset : y);
	fChanged = true;

	_UpdateScrollbar();
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

	// Follow the newly added text (matches the dialogue box scrolling to
	// the latest line). ScrollTo() clamps, so this is a no-op until the
	// content actually overflows.
	ScrollTo(0, _MaxYOffset());

	fChanged = true;
}


void
TextArea::_UpdateScrollbar()
{
	if (fScrollbar != NULL)
		fScrollbar->SetScrollInfo(fYOffset, _MaxYOffset());
}


int16
TextArea::_ContentHeight() const
{
	int16 height = 0;
	for (const TextLine& line : fLines)
		height += line.height + kLineSpacing;
	return height;
}


int16
TextArea::_MaxYOffset() const
{
	int16 overflow = _ContentHeight() - (int16)Height();
	return overflow > 0 ? overflow : 0;
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


