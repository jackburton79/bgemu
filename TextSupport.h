/*
 * TextDrawing.h
 *
 *  Created on: 03/set/2013
 *      Author: stefano
 */

#ifndef TEXTSUPPORT_H_
#define TEXTSUPPORT_H_


#include "SupportDefs.h"

#include <map>
#include <string>


class BAMResource;
#include "IETypes.h"

class Bitmap;

namespace GFX {
	struct rect;
	struct point;
}

class Font {
public:
	Font(const std::string& fontName);
	~Font();

	uint16 StringWidth(const std::string& string, uint16* height = NULL) const;

	void RenderString(std::string string,
					uint32 flags, Bitmap* bitmap,
					bool useBAMPalette = true) const;

	void RenderString(std::string string,
					uint32 flags, Bitmap* bitmap,
					bool useBAMPalette,
					const GFX::point& point) const;

	void RenderString(std::string string,
					uint32 flags, Bitmap* bitmap,
					bool useBAMPalette,
					const GFX::rect& rect) const;

	uint8 TransparentIndex() const { return fTransparentIndex; };
private:
	void _LoadGlyphs(const std::string& fontName);
	void _RenderString(std::string string,
					uint32 flags, Bitmap* bitmap,
					bool useBAMPalette,
					const GFX::rect* rect, const GFX::point* point) const;
	void _PrepareBitmaps(std::string string, uint16& width, uint16& height,
				std::vector<const Bitmap*> *bitmaps = NULL) const;

	typedef std::map<char, Bitmap*> BitmapMap;
	BitmapMap fGlyphs;
	
	uint8 fTransparentIndex;
};


class FontRoster {
public:
	static const Font* GetFont(const std::string& name);

private:
	static std::map<std::string, Font*> sFonts;
};


#endif /* TEXTDRAWING_H_ */
