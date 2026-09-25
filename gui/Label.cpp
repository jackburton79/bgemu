/*
 * Label.cpp
 *
 *  Created on: 20/ott/2012
 *      Author: stefano
 */

#include "BamResource.h"
#include "Bitmap.h"
#include "GraphicsEngine.h"
#include "Label.h"
#include "ResManager.h"
#include "TextSupport.h"
#include "Window.h"


// LABEL_USE_RGB_COLORS labels carry their own ink/background gradient
// (color1/color2) instead of using their font's natural baked-in color -
// confirmed against real GUIINV/GUIREC data (e.g. GUIINV's party-gold
// label: color1 (255, 230, 130) - a gold/tan tone, exactly what a gold
// count should look like) and against GemRB's CHUImporter.cpp, which
// builds its Color from these same 4 bytes in the same R,G,B order (no
// channel swap - alpha is unused for label ink, GemRB hardcodes it to 0
// too). Returns false (palette left untouched) when the flag isn't set,
// so the caller can pass NULL through to RenderString() and fall back to
// the font's own default appearance.
static bool
_BuildLabelPalette(const IE::label* label, GFX::Palette& palette)
{
	if (!(label->flags & IE::LABEL_USE_RGB_COLORS))
		return false;

	const GFX::Color start = { label->color1_r, label->color1_g, label->color1_b, 0 };
	const GFX::Color end = { label->color2_r, label->color2_g, label->color2_b, 0 };
	palette = GFX::Palette(start, end);
	return true;
}


Label::Label(IE::label* label)
	:
	Control(label),
	fBitmap(NULL)
{
	// Only an RGB-colored label's own palette can actually stick - it's
	// the only case rendering onto an 8-bit (indexed) bitmap; a 16-bit
	// one has no palette to override, so SetPalette() no-ops and the
	// glyph blit keeps each glyph's own font-native color regardless.
	int depth = (label->flags & IE::LABEL_USE_RGB_COLORS) ? 8 : 16;
	fBitmap = new Bitmap(label->w, label->h, depth);

	GFX::Palette customPalette;
	const GFX::Palette* renderPalette =
		_BuildLabelPalette(label, customPalette) ? &customPalette : nullptr;

	std::string fontName = label->font_bam.CString();
	std::string text = IDTable::GetDialog(label->text_ref);
	if (!text.empty())
		FontRoster::GetFont(fontName)->RenderString(text, label->flags, fBitmap, renderPalette);
}


Label::~Label()
{
	if (fBitmap != NULL)
		fBitmap->Release();
}


void
Label::SetText(const std::string& text)
{
	fText = text;
	fBitmap->Clear(0);
	if (!text.empty()) {
		IE::label* label = static_cast<IE::label*>(fControl);
		GFX::Palette customPalette;
		const GFX::Palette* renderPalette =
			_BuildLabelPalette(label, customPalette) ? &customPalette : nullptr;
		FontRoster::GetFont(label->font_bam.CString())->RenderString(text, label->flags, fBitmap, renderPalette);
	}
}

/* virtual */
void
Label::MouseMoved(IE::point point, uint32 transit)
{
	Control::MouseMoved(point, transit);

	if (transit == Control::MOUSE_ENTER) {
		NotifyHovered(true);
	} else if (transit == Control::MOUSE_EXIT) {
		NotifyHovered(false);
	}
}

/* virtual */
void
Label::Draw()
{
	GFX::rect destRect = Frame();
	fWindow->ConvertToScreen(destRect);
	GraphicsEngine::Get()->BlitToScreen(fBitmap, NULL, &destRect);
}
