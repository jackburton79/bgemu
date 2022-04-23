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
					const GFX::Palette* palette) const
{
	GFX::rect frame = bitmap->Frame();
	_RenderString(string, flags, bitmap, palette, &frame, NULL);
}


void
Font::RenderString(const std::string& string, uint32 flags, Bitmap* bitmap,
					const GFX::Palette* palette, const GFX::point& point) const
{
	_RenderString(string, flags, bitmap, palette, NULL, &point);
}


void
Font::RenderString(const std::string& string, uint32 flags, Bitmap* bitmap,
					const GFX::Palette* palette, const GFX::rect& rect) const
{
	_RenderString(string, flags, bitmap, palette, &rect, NULL);
}


// Returns a bitmap with the passed string rendered on it
Bitmap*
Font::GetRenderedString(const std::string& string, uint32 flags,
						const GFX::Palette* palette) const
{
	uint16 height;
	uint16 stringWidth = StringWidth(string, &height);
	// TODO: Bitmap is always 8 bits
	::Bitmap* bitmap = new ::Bitmap(stringWidth, height, 8);
	// render the string to a bitmap
	GFX::rect rect(0, 0, bitmap->Width(), bitmap->Height());
	_RenderString(string, 0, bitmap, palette, &rect, NULL);

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
	GFX::rect containerRect = { 0, 0, width, height };

	if (destRect != NULL) {
		if (flags & IE::LABEL_JUSTIFY_CENTER)
			containerRect.x = (destRect->w - width) / 2;
		else if (flags & IE::LABEL_JUSTIFY_RIGHT)
			containerRect.x = destRect->w - width;
		containerRect.x += destRect->x;
		containerRect.y += destRect->y;
	} else if (destPoint != NULL) {
		containerRect.x = destPoint->x;
		containerRect.y = destPoint->y;
	}

	return containerRect;
}


GFX::rect
Font::_CalcGlyphRect(const Glyph& glyph, uint32 flags,
							   const GFX::rect& containerRect) const
{
	sint16 ascent = glyph.bitmap->Frame().y;
	GFX::rect rect;
	rect.x = containerRect.x;
	rect.y = containerRect.y;
	rect.w = glyph.bitmap->Width();
	rect.h = glyph.bitmap->Height();

#if 0
	if (flags & IE::LABEL_JUSTIFY_BOTTOM)
		rect.y = containerRect.h - rect.h;
	else if (flags & IE::LABEL_JUSTIFY_TOP)
		rect.y = 0;
	else {
		// center
		rect.y = (containerRect.h - rect.h) / 2;
	}
#endif
	rect.y += containerRect.h - ascent;
	return rect;
}


void
Font::_RenderString(const std::string& string, uint32 flags, Bitmap* bitmap,
					const GFX::Palette* palette,
					const GFX::rect* destRect,
					const GFX::point* destPoint) const
{
	std::vector<Glyph> glyphs;
	uint16 totalWidth = 0;
	uint16 maxHeight = 0;
	_PrepareGlyphs(string, totalWidth, maxHeight, &glyphs);

	const Bitmap* firstFrame = glyphs.back().bitmap;

	if (palette != NULL) {
		bitmap->SetPalette(*palette);
	} else if (fPalette != NULL) {
		bitmap->SetPalette(*fPalette);
	} else {
		// No palette.
		throw std::runtime_error("Font::RenderString: no palette");
		GFX::Palette framePalette;
		firstFrame->GetPalette(framePalette);
		bitmap->SetPalette(framePalette);
	}

	uint32 colorKey;
	if (firstFrame->GetColorKey(colorKey))
		bitmap->SetColorKey(colorKey);

	// Render glyphs
	GFX::rect containerRect = _GetContainerRect(totalWidth, maxHeight,
												 flags, destRect, destPoint);
	for (std::vector<Glyph>::const_iterator i = glyphs.begin();
			i != glyphs.end(); i++) {
		const Glyph glyph = (*i);

		GFX::rect glyphRect = _CalcGlyphRect(glyph, flags, containerRect);
		GraphicsEngine::BlitBitmap(glyph.bitmap, NULL, bitmap, &glyphRect);

		// Advance cursor
		containerRect.x += glyph.bitmap->Frame().w;
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
