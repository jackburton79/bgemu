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


class TextSupport {
public:
	static void GetTextWidthAndHeight(std::string string,
					BAMResource* fontRes,
					uint32 flags,
					uint32* width = NULL,
					uint32* height = NULL);

	static std::string GetFittingString(std::string string,
					BAMResource* fontRes,
					uint32 flags,
					uint32 maxWidth,
					uint32* fittingWidth = NULL);

	static void RenderString(std::string string,
								const res_ref& fontRes,
								uint32 flags, Bitmap* bitmap);

	static void RenderString(std::string string,
								const res_ref& fontRes,
								uint32 flags, Bitmap* bitmap,
								const GFX::point& point);

	static void RenderString(std::string string,
								BAMResource* fontRes,
								uint32 flags, Bitmap* bitmap);

	static void RenderString(std::string string,
								BAMResource* fontRes,
								uint32 flags, Bitmap* bitmap,
								const GFX::rect& rect);

	static void RenderString(std::string string,
								BAMResource* fontRes,
								uint32 flags, Bitmap* bitmap,
								const GFX::point& point);
private:
	static std::vector<Bitmap*> _PrepareFrames(std::string string, BAMResource* fontRes,
									uint32& totalWidth, uint16& maxHeight);
	static void _RenderBitmaps(std::vector<Bitmap*> frames, Bitmap* bitmap,
									GFX::rect rect, uint32 flags);
};

#endif /* TEXTDRAWING_H_ */
