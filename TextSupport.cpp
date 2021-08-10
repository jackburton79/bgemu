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
	GlyphMap::iterator i;
	for (i = fGlyphs.begin(); i != fGlyphs.end(); i++) {
		i->second.bitmap->Release();
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
	_PrepareGlyphs(string, totalWidth, maxHeight, NULL);
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
		Bitmap* bitmap = fontRes->FrameForCycle(cycleNum, 0);
		if (bitmap != NULL) {
			Glyph glyph = { c, bitmap };
			fGlyphs[c] = glyph;
		} else {
			std::cerr << "glyph not found for *" << int(c) << "*" << std::endl;
			break;
		}
	}
	gResManager->ReleaseResource(fontRes);
}


GFX::rect
Font::_GetFirstGlyphRect(const GFX::rect* destRect, uint32 flags,
							uint16 totalWidth,
							const GFX::point* destPoint) const
{
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

	return rect;
}


void
Font::_AdjustGlyphAlignment(GFX::rect& rect, uint32 flags,
							const GFX::rect& containerRect, const Glyph glyph) const
{
	const Bitmap* bitmap = glyph.bitmap;
	if (flags & IE::LABEL_JUSTIFY_BOTTOM)
		rect.y = containerRect.h - bitmap->Height();
	else if (flags & IE::LABEL_JUSTIFY_TOP)
		rect.y = 0;
	else {
		// center
		rect.y = (containerRect.h - bitmap->Height()) / 2;
	}

	// TODO: improve
	if (glyph.char_code == '"')
		rect.y = 0;
	else if (glyph.char_code == '.' || glyph.char_code == ',')
		rect.y = containerRect.h - bitmap->Height();

	rect.y += containerRect.y;
	rect.w = bitmap->Frame().w;
	rect.h = bitmap->Frame().h;
}


void
Font::_RenderString(const std::string& string, uint32 flags, Bitmap* bitmap,
					bool useBAMPalette,
					const GFX::rect* destRect,
					const GFX::point* destPoint) const
{
	std::vector<Glyph> glyphs;
	uint16 totalWidth = 0;
	uint16 maxHeight = 0;
	_PrepareGlyphs(string, totalWidth, maxHeight, &glyphs);

	if (useBAMPalette) {
		const Bitmap* firstFrame = glyphs.back().bitmap;
		GFX::Palette palette;
		firstFrame->GetPalette(palette);
		bitmap->SetPalette(palette);
#if 0
		uint32 colorKey;
		if (firstFrame->GetColorKey(colorKey))
			bitmap->SetColorKey(colorKey);
#endif
	}

	// Render glyphs
	GFX::rect rect = _GetFirstGlyphRect(destRect, flags, totalWidth, destPoint);
	GFX::rect containerRect(rect.x, rect.y, totalWidth, maxHeight);
	for (std::vector<Glyph>::const_iterator i = glyphs.begin();
			i != glyphs.end(); i++) {
		const Glyph glyph = (*i);
		_AdjustGlyphAlignment(rect, flags, containerRect, glyph);
		GraphicsEngine::BlitBitmap(glyph.bitmap, NULL, bitmap, &rect);

		// Advance cursor
		rect.x += glyph.bitmap->Frame().w;
	}
}


void
Font::_PrepareGlyphs(const std::string& string, uint16& width, uint16& height,
				std::vector<Glyph> *glyphs) const
{
	// First pass: calculate total width and height
	for (std::string::const_iterator c = string.begin();
			c != string.end(); c++) {
		GlyphMap::const_iterator g = fGlyphs.find(*c);
		if (g == fGlyphs.end()) {
			// glyph not found/cached
			continue;
		}
		Glyph newGlyph = g->second;
		width += newGlyph.bitmap->Frame().w;
		height = std::max(newGlyph.bitmap->Frame().h, height);
		if (glyphs != NULL)
			glyphs->push_back(newGlyph);

		//std::cout << "char: " << (char)*c << ", height: " << newFrame->Height() << std::endl;
	}
}


// FontRoster
FontRoster::FontsMap FontRoster::sFonts;


/* static */
const Font*
FontRoster::GetFont(const std::string& name)
{
	static FontsMap::iterator i = sFonts.find(name);
	if (i != sFonts.end())
		return i->second;
	
	Font* font = new Font(name);
	sFonts[name] = font;
	
	return font;
}
