/*
 * TextArea.cpp
 *
 *  Created on: 20/ott/2012
 *      Author: stefano
 */

#include "BamResource.h"
#include "Bitmap.h"
#include "GraphicsEngine.h"
#include "ResManager.h"
#include "Utils.h"
#include "TextArea.h"
#include "TLKResource.h"
#include "Utils.h"
#include "Window.h"


TextArea::TextArea(IE::text_area* text)
	:
	Control(text),
	fBitmap(NULL)
{
	std::cout << "TextArea:" << std::endl;
	fFontResource = gResManager->GetBAM(text->font_bam);
	fBitmap = GraphicsEngine::CreateBitmap(text->w, text->h, 8);

	Palette palette;
	Color start = { text->color1_r, text->color1_g, text->color1_b, text->color1_a };
	Color end = { text->color3_r, text->color3_g, text->color3_b, text->color3_a };
	GraphicsEngine::CreateGradient(end, start, palette);
	fBitmap->SetPalette(palette);
}


TextArea::~TextArea()
{
	gResManager->ReleaseResource(fFontResource);
	GraphicsEngine::DeleteBitmap(fBitmap);
}


/* virtual */
void
TextArea::Draw()
{
	GFX::rect destRect(fControl->x, fControl->y, fControl->w, fControl->h);
	fWindow->ConvertToScreen(destRect);
	GraphicsEngine::Get()->BlitToScreen(fBitmap, NULL, &destRect);
}


void
TextArea::SetText(const char* text)
{
	fBitmap->Clear(0);
	uint32 flags = IE::LABEL_JUSTIFY_CENTER;
	RenderString(text, fFontResource, flags, fBitmap);
}
