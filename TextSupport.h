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

	void RenderString(std::string string,
					uint32 flags, Bitmap* bitmap) const;

	void RenderString(std::string string,
					uint32 flags, Bitmap* bitmap,
					const GFX::point& point) const;

	void RenderString(std::string string,
					uint32 flags, Bitmap* bitmap,
					const GFX::rect& rect) const;

private:
	void _LoadGlyphs(const std::string& fontName);
	void _RenderString(std::string string,
					uint32 flags, Bitmap* bitmap,
					const GFX::rect* rect, const GFX::point* point) const;
	
	typedef std::map<char, Bitmap*> BitmapMap;
	BitmapMap fGlyphs;
};


class FontRoster {
public:
	static Font* GetFont(const std::string& name);

private:
	static std::map<std::string, Font*> sFonts;
};


#endif /* TEXTDRAWING_H_ */
