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
	:
	fName(fontName)
{
	_LoadGlyphs(fName);
}


Font::~Font()
{
	std::map<char, Bitmap*>::iterator i;
	for (i = fGlyphs.begin(); i != fGlyphs.end(); i++) {
		i->second->Release();
	}
}


std::string
Font::Name() const
{
	return fName;
}


uint16
Font::StringWidth(const std::string& string, uint16* height) const
{
	uint16 totalWidth = 0;
	uint16 maxHeight = 0;
	_PrepareBitmaps(string, totalWidth, maxHeight, NULL);
	if (height != NULL)
		*height = maxHeight;
	return totalWidth;
}


std::string
Font::TruncateString(std::string& string, uint16 maxWidth) const
{
	size_t breakPos = string.length();
	std::string line = string;
	for (;;) {
		line = string.substr(0, breakPos);
		if (StringWidth(line, NULL) < maxWidth)
			break;
		breakPos = string.rfind(" ", breakPos - 1);
	}
	if (breakPos == string.length())
		string = "";
	else
		string = string.substr(breakPos + 1, string.length());
	return line;
}


void
Font::RenderString(const std::string& string, uint32 flags, Bitmap* bitmap,
					bool useBAMPalette) const
{
	GFX::rect frame = bitmap->Frame();
	_RenderString(string, flags, bitmap, useBAMPalette, &frame, NULL);
}


void
Font::RenderString(const std::string& string, uint32 flags, Bitmap* bitmap,
					bool useBAMPalette, const GFX::rect& rect) const
{
	_RenderString(string, flags, bitmap, useBAMPalette, &rect, NULL);
}


void
Font::RenderString(const std::string& string, uint32 flags, Bitmap* bitmap,
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
			fGlyphs[c] = glyph;
		} else {
			std::cerr << "glyph not found for *" << int(c) << "*" << std::endl;
			break;
		}
	}
	gResManager->ReleaseResource(fontRes);
}


void
Font::_RenderString(const std::string& string, uint32 flags, Bitmap* bitmap,
					bool useBAMPalette,
					const GFX::rect* destRect,
					const GFX::point* destPoint) const
{
	std::vector<const Bitmap*> frames;
	uint16 totalWidth = 0;
	uint16 maxHeight = 0;
	_PrepareBitmaps(string, totalWidth, maxHeight, &frames);

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

	GFX::rect containerRect = {
		rect.x,
		rect.y,
		totalWidth,
		maxHeight
	};

	if (useBAMPalette) {
		const Bitmap* firstFrame = frames.back();
		GFX::Palette palette;
		firstFrame->GetPalette(palette);
		bitmap->SetPalette(palette);
#if 0
		uint32 colorKey;
		if (firstFrame->GetColorKey(colorKey))
			bitmap->SetColorKey(colorKey);
#endif
	}
	// Render the glyphs
	for (std::vector<const Bitmap*>::const_iterator i = frames.begin();
			i != frames.end(); i++) {
		const Bitmap* glyph = *i;
		if (flags & IE::LABEL_JUSTIFY_BOTTOM)
			rect.y = containerRect.h - glyph->Height();
		else if (flags & IE::LABEL_JUSTIFY_TOP)
			rect.y = 0;
		else
			rect.y = (containerRect.h - glyph->Height()) / 2;
		rect.y += containerRect.y;

		rect.w = glyph->Frame().w;
		rect.h = glyph->Frame().h;
		GraphicsEngine::BlitBitmap(glyph, NULL, bitmap, &rect);
		rect.x += glyph->Frame().w;
	}
}


void
Font::_PrepareBitmaps(const std::string& string, uint16& width, uint16& height,
				std::vector<const Bitmap*> *bitmaps) const
{
	// First pass: calculate total width and height
	for (std::string::const_iterator c = string.begin();
			c != string.end(); c++) {
		std::map<char, Bitmap*>::const_iterator g = fGlyphs.find(*c);
		if (g == fGlyphs.end()) {
			// glyph not found/cached
			continue;
		}
		const Bitmap* newFrame = g->second;
		width += newFrame->Frame().w;
		height = std::max(newFrame->Frame().h, height);
		if (bitmaps != NULL)
			bitmaps->push_back(newFrame);
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
