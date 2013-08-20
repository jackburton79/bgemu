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
#include "Window.h"

TextArea::TextArea(IE::text_area* text)
	:
	Control(text),
	fBitmap(NULL)
{
	std::cout << "TextArea:" << std::endl;
	fFontResource = gResManager->GetBAM(text->font_bam);
	fBitmap = GraphicsEngine::CreateBitmap(text->w, text->h, 16);
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
	GFX::rect destRect(0, 0, fControl->w, fControl->h);
	destRect.x = fWindow->Position().x + fControl->x;
	destRect.y = fWindow->Position().y + fControl->y;
	GraphicsEngine::Get()->BlitToScreen(fBitmap, NULL, &destRect);
}


void
TextArea::SetText(const char* text)
{
	fBitmap->Clear(0);
	uint32 flags = IE::LABEL_JUSTIFY_CENTER;
	RenderString(text, fFontResource, flags, fBitmap);
}
