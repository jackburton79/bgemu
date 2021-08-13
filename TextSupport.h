/*
 * TextDrawing.h
 *
 *  Created on: 03/set/2013
 *      Author: stefano
 */

#ifndef TEXTSUPPORT_H_
#define TEXTSUPPORT_H_


#include "IETypes.h"
#include "SupportDefs.h"

#include <map>
#include <string>

class BAMResource;
class Bitmap;

namespace GFX {
	struct rect;
	struct point;
	class Palette;
}

class Font {
public:
	Font(const std::string& fontName);
	~Font();

	std::string Name() const;
	uint16 Height() const;
	uint16 StringWidth(const std::string& string, uint16* height = NULL) const;

	std::string TruncateString(std::string& string, uint16 maxWidth, uint16* truncatedWidth = NULL) const;

	void RenderString(const std::string& string,
					uint32 flags, Bitmap* bitmap,
					bool useBAMPalette = true) const;

	void RenderString(const std::string& string,
					uint32 flags, Bitmap* bitmap,
					bool useBAMPalette,
					const GFX::point& point) const;

	void RenderString(const std::string& string,
					uint32 flags, Bitmap* bitmap,
					bool useBAMPalette,
					const GFX::rect& rect) const;

	uint8 TransparentIndex() const { return fTransparentIndex; };

	struct Glyph {
		int char_code;
		Bitmap* bitmap;
	};

private:
	void _LoadGlyphs(const std::string& fontName);
	void _RenderString(const std::string& string,
					uint32 flags, Bitmap* bitmap,
					bool useBAMPalette,
					const GFX::rect* rect, const GFX::point* point) const;
	void _PrepareGlyphs(const std::string& string, uint16& width, uint16& height,
				std::vector<Glyph> *glyphs = NULL) const;
	GFX::rect _GetFirstGlyphRect(const GFX::rect* destRect, uint32 flags,
						uint16 totalWidth, const GFX::point* destPoint) const;
	void _AdjustGlyphAlignment(GFX::rect& rect, uint32 flags,
							   const GFX::rect& containerRect,
							   const Glyph glyph) const;

	std::string fName;

	typedef std::map<char, Glyph> GlyphMap;
	GlyphMap fGlyphs;
	uint16 fHeight;
	GFX::Palette* fPalette;

	uint8 fTransparentIndex;
};


class FontRoster {
public:
	static const Font* GetFont(const std::string& name);
	static void Destroy();

private:
	typedef std::map<std::string, Font*> FontsMap;
	static FontsMap sFonts;
};


#endif /* TEXTDRAWING_H_ */
