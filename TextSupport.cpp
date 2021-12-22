/*
 * TextDrawing.cpp
 *
 *  Created on: 03/set/2013
 *      Author: Stefano Ceccherini
 */

#include "TextSupport.h"

#include "BamResource.h"
#include "GraphicsEngine.h"
#include "Log.h"
#include "Path.h"
#include "ResManager.h"

#include <algorithm>
#include <limits.h>


static uint32
cycle_num_for_char(int c)
{
	return c - 1;
}


enum char_classification {
	CHAR_IS_NORMAL = 0,
	CHAR_IS_TOP,
	CHAR_IS_BOTTOM
};


static uint32
get_char_classification(int c)
{
	switch (c) {
		case ',':
		case '.':
			return CHAR_IS_BOTTOM;
		case '"':
		case '\'':
			return CHAR_IS_TOP;
	}

	return CHAR_IS_NORMAL;
}


Font::Font(const std::string& fontName)
	:
	fName(fontName),
	fHeight(0),
	fPalette(NULL),
	fTransparentIndex(0)
{
	_LoadGlyphs(fName);
/*
	const GFX::Color colorStart = {
		0,
		0,
		0,
		0
	};
	const GFX::Color colorEnd = {
		255,
		255,
		255,
		0
	};
	fPalette = new GFX::Palette(colorStart, colorEnd);*/
	fPalette = new GFX::Palette(*GFX::kPaletteYellow);
}


Font::Font(const Font& font)
	:
	fName(font.fName),
	fHeight(font.Height()),
	fPalette(NULL),
	fTransparentIndex(font.fTransparentIndex)
{
	// private unimplemented
}


Font::~Font()
{
	GlyphMap::iterator i;
	for (i = fGlyphs.begin(); i != fGlyphs.end(); i++) {
		i->second.bitmap->Release();
	}
	delete fPalette;
}


std::string
Font::Name() const
{
	return fName;
}


uint16
Font::Height() const
{
	return fHeight;
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
Font::TruncateString(std::string& string, uint16 maxWidth, uint16* truncatedWidth) const
{
	std::string line = string;
	size_t breakPos = string.length();
	for (;;) {
		line = string.substr(0, breakPos);
		uint16 newWidth = StringWidth(line, NULL);
		if (newWidth < maxWidth) {
			if (truncatedWidth != NULL)
				*truncatedWidth = newWidth;
			break;
		}
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


// Returns a bitmap with the passed string rendered on it
Bitmap*
Font::GetRenderedString(const std::string& string, uint32 flags) const
{
	uint16 height;
	uint16 stringWidth = StringWidth(string, &height);
	// TODO: Bitmap is always 8 bits
	::Bitmap* bitmap = new ::Bitmap(stringWidth, height, 8);
	// render the string to a bitmap
	GFX::rect rect(0, 0, bitmap->Width(), bitmap->Height());
	RenderString(string, 0, bitmap, true, rect);

	return bitmap;
}


void
Font::_LoadGlyphs(const std::string& fontName)
{
	BAMResource* fontRes = gResManager->GetBAM(fontName.c_str());
	if (fontRes == NULL) {
		std::cerr << Log::Red << "Font::_LoadGlyphs(): no resource ";
		std::cerr << fontName << Log::Normal << std::endl;
		return;
	}
	fTransparentIndex = fontRes->TransparentIndex();
	for (int c = 1; c < 256; c++) {
		uint32 cycleNum = cycle_num_for_char(c);
		Bitmap* bitmap = fontRes->FrameForCycle(cycleNum, 0);
		if (bitmap != NULL) {
			Glyph glyph = { c, bitmap };
			fGlyphs[c] = glyph;
			fHeight = std::max(bitmap->Height(), fHeight);
		} else {
			std::cerr << Log::Yellow << "Font::_LoadGlyphs(): glyph not found for *";
			std::cerr << int(c) << "*" << Log::Normal << std::endl;
			break;
		}
	}
	gResManager->ReleaseResource(fontRes);
}


GFX::rect
Font::_GetContainerRect(uint16 width, uint16 height,
						 uint32 flags,
						 const GFX::rect* destRect,
						 const GFX::point* destPoint) const
{
	GFX::rect rect;
	rect.w = width;
	rect.h = height;

	if (destRect != NULL) {
		if (flags & IE::LABEL_JUSTIFY_CENTER)
			rect.x = (destRect->w - width) / 2;
		else if (flags & IE::LABEL_JUSTIFY_RIGHT)
			rect.x = destRect->w - width;
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

	rect.w = bitmap->Frame().w;
	rect.h = bitmap->Frame().h;

	if (flags & IE::LABEL_JUSTIFY_BOTTOM)
		rect.y = containerRect.h - bitmap->Height();
	else if (flags & IE::LABEL_JUSTIFY_TOP)
		rect.y = 0;
	else {
		// center
		rect.y = (containerRect.h - bitmap->Height()) / 2;
	}

	// TODO: improve
	uint32 charClassification = get_char_classification(glyph.char_code);
	if (charClassification == CHAR_IS_TOP)
		rect.y = 0;
	else if (charClassification == CHAR_IS_BOTTOM)
		rect.y = containerRect.h - bitmap->Height();

	rect.y += containerRect.y;
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

	if (fPalette != NULL)
		bitmap->SetPalette(*fPalette);

	const Bitmap* firstFrame = glyphs.back().bitmap;

	/*if (useBAMPalette) {

		GFX::Palette palette;
		firstFrame->GetPalette(palette);
		bitmap->SetPalette(palette);
	}*/

	uint32 colorKey;
	if (firstFrame->GetColorKey(colorKey))
		bitmap->SetColorKey(colorKey);

	// Render glyphs
	GFX::rect containerRect = _GetContainerRect(totalWidth, maxHeight,
												 flags, destRect, destPoint);
	GFX::rect rect = containerRect;
	for (std::vector<Glyph>::const_iterator i = glyphs.begin();
			i != glyphs.end(); i++) {
		const Glyph glyph = (*i);

		_AdjustGlyphAlignment(rect, flags, containerRect, glyph);
		GraphicsEngine::BlitBitmap(glyph.bitmap, NULL, bitmap, &rect);

		// Advance cursor
		rect.x += glyph.bitmap->Frame().w;
	}

	if (flags & TEXT_SELECTED)
		bitmap->FillRect(containerRect, 10);
}


void
Font::_PrepareGlyphs(const std::string& string, uint16& width, uint16& height,
				std::vector<Glyph> *glyphs) const
{
	// calculate total width and height
	for (std::string::const_iterator c = string.begin();
			c != string.end(); c++) {
		GlyphMap::const_iterator g = fGlyphs.find(*c);
		if (g == fGlyphs.end()) {
			// glyph not found/cached
			std::cerr << Log::Yellow << "Font::_PrepareGlyphs: glyph '";
			std::cerr << (char)*c << "' not found" << Log::Normal << std::endl;
			continue;
		}
		Glyph newGlyph = g->second;
		width += newGlyph.bitmap->Frame().w;
		height = std::max(newGlyph.bitmap->Frame().h, height);
		if (glyphs != NULL)
			glyphs->push_back(newGlyph);
	}
}


// FontRoster
FontRoster::FontsMap FontRoster::sFonts;


/* static */
const Font*
FontRoster::GetFont(const std::string& name)
{
	const FontsMap::iterator i = sFonts.find(name);
	if (i != sFonts.end())
		return i->second;
	
	Font* font = new Font(name);
	sFonts[name] = font;
	
	return font;
}


/* static */
void
FontRoster::Destroy()
{
	for (FontsMap::iterator i = sFonts.begin(); i != sFonts.end(); i++)
		delete i->second;
}
