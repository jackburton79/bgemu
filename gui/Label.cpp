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
#include "Window.h"


Label::Label(IE::label* label)
	:
	Control(label),
	fBitmap(NULL)
{
	fFontResource = gResManager->GetBAM(label->font_bam);

	int depth = 16;
	if (label->flags & IE::LABEL_USE_RGB_COLORS)
		depth = 8;

	fBitmap = GraphicsEngine::CreateBitmap(label->w, label->h, depth);

	if (depth == 8) {
		const Color colorStart = {
			label->color1_r,
			label->color1_g,
			label->color1_b,
			label->color1_a
		};
		const Color colorEnd = {
			label->color2_r,
			label->color2_g,
			label->color2_b,
			label->color2_a
		};

		_SetPalette(colorStart, colorEnd);
	}

	TextSupport::RenderString(IDTable::GetDialog(label->text_ref), fFontResource, label->flags, fBitmap);
}


Label::~Label()
{
	GraphicsEngine::DeleteBitmap(fBitmap);
	gResManager->ReleaseResource(fFontResource);
}


void
Label::SetText(const char* text)
{
	fBitmap->Clear(0);
	IE::label* label = static_cast<IE::label*>(fControl);
	TextSupport::RenderString(text, fFontResource, label->flags, fBitmap);
}


/* virtual */
void
Label::Draw()
{
	GFX::rect destRect(fControl->x, fControl->y, fControl->w, fControl->h);
	fWindow->ConvertToScreen(destRect);
	GraphicsEngine::Get()->BlitToScreen(fBitmap, NULL, &destRect);
}


void
Label::_SetPalette(const Color& start, const Color& end)
{
	Palette palette;
	GraphicsEngine::CreateGradient(start, end, palette);
	fBitmap->SetColors(palette.colors, 0, 256);
}
