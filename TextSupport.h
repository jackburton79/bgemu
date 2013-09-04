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
class TextSupport {
public:
	static void RenderString(std::string string,
								const res_ref& fontRes,
								uint32 flags, Bitmap* bitmap);
	static void RenderString(std::string string,
								BAMResource* fontRes,
								uint32 flags, Bitmap* bitmap);
};

#endif /* TEXTDRAWING_H_ */
