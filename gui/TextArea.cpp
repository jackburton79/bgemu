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
	//fInitialsFontResource(NULL),
	fBitmap(NULL)
{
	fBitmap = new Bitmap(text->w, text->h, 8);

	GFX::Palette palette;
	GFX::Color color1 = { text->color1_r, text->color1_g, text->color1_b, text->color1_a };
	GFX::Color color2 = { text->color2_r, text->color2_g, text->color2_b, text->color2_a };
	GFX::Color color3 = { text->color3_r, text->color3_g, text->color3_b, text->color3_a };

	GFX::Color& start = color3;
	GFX::Color& end = color1;
	GFX::Color& transparent = color2;
	GraphicsEngine::CreateGradient(end, start, palette);
	std::cout << std::dec << (int)text->color1_r << ", ";
	std::cout << (int)text->color1_g << ", " << (int)text->color1_b << std::endl;
	std::cout << std::dec << (int)text->color2_r << ", ";
	std::cout << (int)text->color2_g << ", " << (int)text->color2_b << std::endl;
	std::cout << std::dec << (int)text->color3_r << ", ";
	std::cout << (int)text->color3_g << ", " << (int)text->color3_b << std::endl;
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
	GFX::rect destRect(fControl->x, fControl->y, fControl->w, fControl->h);
	fWindow->ConvertToScreen(destRect);
	GraphicsEngine::Get()->BlitToScreen(fBitmap, NULL, &destRect);
}


void
TextArea::AddText(const char* text)
{
}


void
TextArea::SetText(const char* text)
{
	// TODO: Write initials with the correct font
	std::string fontName = ((IE::text_area*)fControl)->font_bam.CString();
	fBitmap->Clear(0);
	uint32 flags = IE::LABEL_JUSTIFY_LEFT;
	FontRoster::GetFont(fontName)->RenderString(text, flags, fBitmap, false);
}
