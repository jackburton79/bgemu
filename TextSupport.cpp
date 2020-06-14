/*
 * TextDrawing.cpp
 *
 *  Created on: 03/set/2013
 *      Author: Stefano Ceccherini
 */

#include "TextSupport.h"

#include "BamResource.h"
#include "GraphicsEngine.h"
#include "Path.h"
#include "ResManager.h"


#include <algorithm>
#include <limits.h>


static uint32
cycle_num_for_char(int c)
{
	return c - 1;
}


Font::Font(const std::string& fontName)
{
	_LoadGlyphs(fontName);
}


Font::~Font()
{
	std::map<char, Bitmap*>::iterator i;
	for (i = fGlyphs.begin(); i != fGlyphs.end(); i++) {
		i->second->Release();
	}
}


uint16
Font::StringWidth(const std::string& string, uint16* height) const
{
	uint16 totalWidth = 0;
	uint16 maxHeight = 0;
	// TODO: Duplicate code here and in _RenderString()
	for (std::string::const_iterator c = string.begin();
			c != string.end(); c++) {
		std::map<char, Bitmap*>::const_iterator g = fGlyphs.find(*c);
		if (g == fGlyphs.end()) {
			// glyph not found/cached
			continue;
		}
		Bitmap* newFrame = g->second;
		totalWidth += newFrame->Frame().w;
		maxHeight = std::max(newFrame->Frame().h, maxHeight);
	}
	if (height != NULL)
		*height = maxHeight;
	return totalWidth;
}


void
Font::RenderString(std::string string, uint32 flags, Bitmap* bitmap,
					bool useBAMPalette) const
{
	GFX::rect frame = bitmap->Frame();
	_RenderString(string, flags, bitmap, useBAMPalette, &frame, NULL);
}


void
Font::RenderString(std::string string, uint32 flags, Bitmap* bitmap,
					bool useBAMPalette, const GFX::rect& rect) const
{
	_RenderString(string, flags, bitmap, useBAMPalette, &rect, NULL);
}


void
Font::RenderString(std::string string, uint32 flags, Bitmap* bitmap,
					bool useBAMPalette, const GFX::point& point) const
{
	_RenderString(string, flags, bitmap, useBAMPalette, NULL, &point);
}


void
Font::_LoadGlyphs(const std::string& fontName)
{
	BAMResource* fontRes = gResManager->GetBAM(fontName.c_str());
	if (fontRes == NULL)
		return;
	fTransparentIndex = fontRes->TransparentIndex();
	for (int c = 32; c < 256; c++) {
		uint32 cycleNum = cycle_num_for_char(c);
		Bitmap* glyph = fontRes->FrameForCycle(cycleNum, 0);
		if (glyph != NULL) {
			glyph->SetColorKey(fTransparentIndex, true);
			fGlyphs[c] = glyph;
		} else {
			std::cerr << "glyph not found for *" << int(c) << "*" << std::endl;
			break;
		}
	}
	gResManager->ReleaseResource(fontRes);
}


void
Font::_RenderString(std::string string, uint32 flags, Bitmap* bitmap,
					bool useBAMPalette,
					const GFX::rect* destRect,
					const GFX::point* destPoint) const
{
	std::vector<const Bitmap*> frames;
	uint16 totalWidth = 0;
	uint16 maxHeight = 0;
	// First pass: calculate total width and height
	for (std::string::const_iterator c = string.begin();
			c != string.end(); c++) {
		std::map<char, Bitmap*>::const_iterator g = fGlyphs.find(*c);
		if (*c == ' ')
			totalWidth += 10;
		if (g == fGlyphs.end()) {
			std::cout << "*" << int(*c) << "* not found in glyph cache!" << std::endl;
			// glyph not found/cached
			continue;
		}
		const Bitmap* newFrame = g->second;
		totalWidth += newFrame->Frame().w;
		maxHeight = std::max(newFrame->Frame().h, maxHeight);
		frames.push_back(newFrame);
	}

	GFX::rect rect;
	if (destRect != NULL) {
		if (flags & IE::LABEL_JUSTIFY_CENTER)
			rect.x = (destRect->w - totalWidth) / 2;
		else if (flags & IE::LABEL_JUSTIFY_RIGHT)
			rect.x = destRect->w - totalWidth;
		rect.x += destRect->x;
		rect.y += destRect->y;
	} else if (destPoint != NULL) {
		rect.x = destPoint->x;
		rect.y = destPoint->y;
	}

	if (useBAMPalette) {
		const Bitmap* firstFrame = frames.back();
		GFX::Palette palette;
		firstFrame->GetPalette(palette);
		bitmap->SetPalette(palette);
	}
	// Render the glyphs
	for (std::vector<const Bitmap*>::const_iterator i = frames.begin();
			i != frames.end(); i++) {
		const Bitmap* glyph = *i;
		if (destRect != NULL) {
			if (flags & IE::LABEL_JUSTIFY_BOTTOM)
				rect.y = destRect->h - glyph->Height();
			else if (flags & IE::LABEL_JUSTIFY_TOP)
				rect.y = 0;
			else
				rect.y = (destRect->h - glyph->Height()) / 2;
			rect.y += destRect->y;
		}

		rect.w = glyph->Frame().w;
		rect.h = glyph->Frame().h;
		GraphicsEngine::BlitBitmap(glyph, NULL, bitmap, &rect);
		rect.x += glyph->Frame().w;
	}
}

					
// FontRoster
std::map<std::string, Font*> FontRoster::sFonts;


/* static */
const Font*
FontRoster::GetFont(const std::string& name)
{
	static std::map<std::string, Font*>::iterator i = sFonts.find(name);
	if (i != sFonts.end())
		return i->second;
	
	Font* font = new Font(name);
	sFonts[name] = font;
	
	return font;
}
