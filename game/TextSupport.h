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

#include <climits>
#include <map>
#include <string>

class BAMResource;
class Bitmap;

namespace GFX {
	struct rect;
	struct point;
	class Palette;
}

enum text_attributes {
	TEXT_NONE = 0,
	TEXT_SELECTED = 4096
};


class Font {
public:
	Font(const std::string& fontName);
	~Font();

	std::string Name() const;
	uint16 Height() const;
	uint16 StringWidth(const std::string& string, uint16* height = NULL) const;

	std::string TruncateString(std::string& string, uint16 maxWidth, uint16* truncatedWidth = NULL) const;

	// The RenderString methods only work if the destination
	// bitmap is 8-bit paletted
	void RenderString(const std::string& string,
					uint32 flags, Bitmap* bitmap,
					const GFX::Palette* palette = NULL) const;

	void RenderString(const std::string& string,
					uint32 flags, Bitmap* bitmap,
					const GFX::Palette* palette,
					const GFX::point& point,
					const uint32 maxWidth = UINT_MAX) const;

	Bitmap* GetRenderedString(const std::string& string,
							  uint32 flags,
							  const GFX::Palette* palette = NULL) const;

	uint8 TransparentIndex() const { return fTransparentIndex; };

	struct Glyph {
		int char_code;
		Bitmap* bitmap;
	};

private:
	Font(const Font&);

	void _LoadGlyphs(const std::string& fontName);
	void _RenderString(const std::string& string,
					uint32 flags, Bitmap* bitmap,
					const GFX::Palette* palette,
					const GFX::point& point,
					const uint32 maxWidth) const;
	void _PrepareGlyphs(const std::string& string, uint16& width, uint16& height,
				std::vector<Glyph> *glyphs = NULL) const;
	GFX::rect _GetContainerRect(uint16 textWidth,
								 uint32 flags,
								 const GFX::point& destPoint,
								 uint16 width, uint16 height) const;
	GFX::rect _CalcGlyphRect(const Glyph& glyph, uint32 flags,
							   const GFX::rect& containerRect) const;

	std::string fName;

	typedef std::map<char, Glyph> GlyphMap;
	GlyphMap fGlyphs;
	uint16 fHeight;
	uint16 fBaseLine;
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
