/*
 * TextDrawing.h
 *
 *  Created on: 03/set/2013
 *      Author: stefano
 */

#ifndef TEXTSUPPORT_H_
#define TEXTSUPPORT_H_

#include "IETypes.h"

#include <string>

class BAMResource;
class Bitmap;
struct Color;
struct Palette;

namespace GFX {
	struct rect;
}

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
								BAMResource* fontRes,
								uint32 flags, Bitmap* bitmap);
	static void RenderString(std::string string,
								BAMResource* fontRes,
								uint32 flags, Bitmap* bitmap,
								const GFX::rect& rect);
};

#endif /* TEXTDRAWING_H_ */
