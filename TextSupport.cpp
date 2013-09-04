/*
 * TextDrawing.cpp
 *
 *  Created on: 03/set/2013
 *      Author: stefano
 */

#include "TextSupport.h"

#include "BamResource.h"
#include "GraphicsEngine.h"
#include "Path.h"
#include "ResManager.h"


#include <algorithm>
#include <limits.h>


// TODO: Move RenderString to its own file, or a different file
static uint32
cycle_num_for_char(char c)
{
	return (int)c - 1;
}


void
TextSupport::RenderString(std::string string, const res_ref& fontRes,
		uint32 flags, Bitmap* bitmap)
{
	BAMResource* fontResource = gResManager->GetBAM(fontRes);
	if (fontResource != NULL) {
		RenderString(string, fontResource, flags, bitmap);
		gResManager->ReleaseResource(fontResource);
	}
}


void
TextSupport::RenderString(std::string string, BAMResource* fontResource,
		uint32 flags, Bitmap* bitmap)
{
	try {
		std::vector<const Bitmap*> frames;
		uint32 totalWidth = 0;
		uint16 maxHeight = 0;
		for (std::string::iterator i = string.begin();
				i != string.end(); i++) {
			uint32 cycleNum = cycle_num_for_char(*i);
			const Bitmap* newFrame = fontResource->FrameForCycle(cycleNum, 0);
			totalWidth += newFrame->Frame().w;
			maxHeight = std::max(newFrame->Frame().h, maxHeight);
			frames.push_back(newFrame);
		}

		GFX::rect rect;
		if (flags & IE::LABEL_JUSTIFY_CENTER)
			rect.x = (bitmap->Width() - totalWidth) / 2;
		else if (flags & IE::LABEL_JUSTIFY_RIGHT)
			rect.x = bitmap->Width() - totalWidth;

		for (std::vector<const Bitmap*>::const_iterator i = frames.begin();
				i != frames.end(); i++) {
			const Bitmap* letter = *i;
			if (flags & IE::LABEL_JUSTIFY_BOTTOM)
				rect.y = bitmap->Height() - letter->Height();
			else if (flags & IE::LABEL_JUSTIFY_TOP)
				rect.y = 0;
			else
				rect.y = (bitmap->Height() - letter->Height()) / 2;
			rect.w = letter->Frame().w;
			rect.h = letter->Frame().h;

			GraphicsEngine::BlitBitmap(letter, NULL, bitmap, &rect);
			rect.x += letter->Frame().w;
		}
	} catch (...) {
		std::cerr << "RenderString() exception" << std::endl;
	}
}
