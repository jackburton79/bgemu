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
#include "Utils.h"
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

	TLKEntry *textEntry = Dialogs()->EntryAt(label->text_ref);
	RenderString(textEntry->string, fFontResource, label->flags, fBitmap);
	delete textEntry;
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
	RenderString(text, fFontResource, label->flags, fBitmap);
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
	uint8 rFactor = (start.r - end.r) / 255;
	uint8 gFactor = (start.g - end.g) / 255;
	uint8 bFactor = (start.b - end.b) / 255;
	uint8 aFactor = (start.a - end.a) / 255;
	Color* colors = new Color[256];
	colors[0] = start;
	colors[255] = end;
	for (uint8 c = 1; c < 255; c++) {
		colors[c].r = colors[c - 1].r + rFactor;
		colors[c].g = colors[c - 1].g + gFactor;
		colors[c].b = colors[c - 1].b + bFactor;
		colors[c].a = colors[c - 1].a + aFactor;
	}
	fBitmap->SetColors(colors, 0, 256);
	delete[] colors;
}
