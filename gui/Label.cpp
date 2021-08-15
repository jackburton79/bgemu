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
#include "Reference.h"
#include "ResManager.h"
#include "TextSupport.h"
#include "TLKResource.h"
#include "Utils.h"
#include "Window.h"


Label::Label(IE::label* label)
	:
	Control(label),
	fBitmap(NULL)
{
	int depth = 16;
	if (label->flags & IE::LABEL_USE_RGB_COLORS)
		depth = 8;

	fBitmap = new Bitmap(label->w, label->h, depth);

	if (depth == 8) {/*
		const GFX::Color colorStart = {
			label->color1_r,
			label->color1_g,
			label->color1_b,
			label->color1_a
		};
		const GFX::Color colorEnd = {
			label->color2_r,
			label->color2_g,
			label->color2_b,
			label->color2_a
		};

		_SetPalette(colorStart, colorEnd);*/

		// TODO: Should use these colors, but how to build the palette ?
		fBitmap->SetPalette(*GFX::kPaletteYellow);
	}

	std::string fontName = label->font_bam.CString();
	FontRoster::GetFont(fontName)->RenderString(IDTable::GetDialog(label->text_ref), label->flags, fBitmap);
}


Label::~Label()
{
	if (fBitmap != NULL)
		fBitmap->Release();
}


void
Label::SetText(const std::string& text)
{
	fBitmap->Clear(0);
	if (!text.empty()) {
		IE::label* label = static_cast<IE::label*>(fControl);
		FontRoster::GetFont(label->font_bam.CString())->RenderString(text, label->flags, fBitmap);
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
