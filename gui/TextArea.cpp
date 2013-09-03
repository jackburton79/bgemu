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
#include "TextSupport.h"
#include "TLKResource.h"
#include "Utils.h"
#include "Window.h"


TextArea::TextArea(IE::text_area* text)
	:
	Control(text),
	fFontResource(NULL),
	fInitialsFontResource(NULL),
	fBitmap(NULL)
{
	fFontResource = gResManager->GetBAM(text->font_bam);
	fBitmap = GraphicsEngine::CreateBitmap(text->w, text->h, 8);

	Palette palette;
	Color start = { text->color3_r, text->color3_g, text->color3_b, text->color3_a };
	Color end = { text->color2_r, text->color2_g, text->color2_b, text->color2_a };
	GraphicsEngine::CreateGradient(end, start, palette);
	std::cout << std::dec << (int)text->color1_r << ", ";
	std::cout << (int)text->color1_g << ", " << (int)text->color1_b << std::endl;
	std::cout << std::dec << (int)text->color2_r << ", ";
	std::cout << (int)text->color2_g << ", " << (int)text->color2_b << std::endl;
	std::cout << std::dec << (int)text->color3_r << ", ";
	std::cout << (int)text->color3_g << ", " << (int)text->color3_b << std::endl;
	fBitmap->SetPalette(palette);
	fBitmap->SetColorKey(text->color2_r, text->color2_g, text->color2_b, true);
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
	uint32 flags = IE::LABEL_JUSTIFY_LEFT;
	TextSupport::RenderString(text, fFontResource, flags, fBitmap);
}
