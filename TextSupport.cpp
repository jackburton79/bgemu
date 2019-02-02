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


static uint32
cycle_num_for_char(char c)
{
	return (int)c - 1;
}

/* static */
void
TextSupport::GetTextWidthAndHeight(std::string string,
							BAMResource* fontRes,
							uint32 flags, uint32* width, uint32* height)
{
	uint32 totalWidth = 0;
	uint16 maxHeight = 0;
	try {
		std::vector<const Bitmap*> frames;
		for (std::string::iterator i = string.begin();
				i != string.end(); i++) {
			uint32 cycleNum = cycle_num_for_char(*i);
			Bitmap* newFrame = fontRes->FrameForCycle(cycleNum, 0);
			totalWidth += newFrame->Frame().w;
			maxHeight = std::max(newFrame->Frame().h, maxHeight);
			newFrame->Release();
		}
	} catch (...) {
		std::cerr << "StringWidth() exception" << std::endl;
	}
	if (width != NULL)
		*width = totalWidth;
	if (height != NULL)
		*height = (uint32)maxHeight;
}


std::string
TextSupport::GetFittingString(std::string string,
					BAMResource* fontRes,
					uint32 flags,
					uint32 maxWidth,
					uint32* fittingWidth)
{
	uint32 currentWidth = 0;
	uint16 maxHeight = 0;
	std::string fittingString;
	try {
		std::vector<const Bitmap*> frames;
		for (std::string::iterator i = string.begin();
				i != string.end(); i++) {
			uint32 cycleNum = cycle_num_for_char(*i);
			Bitmap* newFrame = fontRes->FrameForCycle(cycleNum, 0);
			uint32 charWidth = newFrame->Frame().w;
			if (currentWidth + charWidth > maxWidth) {
				if (fittingWidth != NULL)
					*fittingWidth = currentWidth;

				break;
			}
			currentWidth += charWidth;
			maxHeight = std::max(newFrame->Frame().h, maxHeight);
			newFrame->Release();
		}
	} catch (...) {
		std::cerr << "StringWidth() exception" << std::endl;
	}

	return fittingString;
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
TextSupport::RenderString(std::string string, const res_ref& fontRes,
		uint32 flags, Bitmap* bitmap, const GFX::point& point)
{
	BAMResource* fontResource = gResManager->GetBAM(fontRes);
	if (fontResource != NULL) {
		RenderString(string, fontResource, flags, bitmap, point);
		gResManager->ReleaseResource(fontResource);
	}
}


void
TextSupport::RenderString(std::string string, BAMResource* fontResource,
		uint32 flags, Bitmap* bitmap)
{
	RenderString(string, fontResource, flags, bitmap, bitmap->Frame());
}


void
TextSupport::RenderString(std::string string,
								BAMResource* fontRes,
								uint32 flags, Bitmap* bitmap,
								const GFX::rect& destRect)
{
	try {
		uint32 totalWidth = 0;
		uint16 maxHeight = 0;
		std::vector<Bitmap*> frames = _PrepareFrames(string, fontRes,
						totalWidth, maxHeight);
		GFX::rect rect;
		if (flags & IE::LABEL_JUSTIFY_CENTER)
			rect.x = (destRect.w - totalWidth) / 2;
		else if (flags & IE::LABEL_JUSTIFY_RIGHT)
			rect.x = destRect.w - totalWidth;

		rect.x += destRect.x;
		rect.y += destRect.y;
		_RenderBitmaps(frames, bitmap, rect, flags);
	} catch (...) {
		std::cerr << "RenderString() exception" << std::endl;
	}
}


void
TextSupport::RenderString(std::string string,
								BAMResource* fontRes,
								uint32 flags, Bitmap* bitmap,
								const GFX::point& point)
{
	try {
		uint32 totalWidth = 0;
		uint16 maxHeight = 0;
		std::vector<Bitmap*> frames = _PrepareFrames(string, fontRes,
						totalWidth, maxHeight);
		GFX::rect rect;
		rect.x = point.x;
		rect.y = point.y;
		rect.w = totalWidth;
		rect.h = maxHeight;
		
		// flags are ignored, render always as justify left
		_RenderBitmaps(frames, bitmap, rect, 0);
	} catch (...) {
		std::cerr << "RenderString() exception" << std::endl;
	}
}


/* static */
std::vector<Bitmap*>
TextSupport::_PrepareFrames(std::string string, BAMResource* fontRes,
							uint32& totalWidth, uint16& maxHeight)
{
	std::vector<Bitmap*> frames;
	totalWidth = 0;
	maxHeight = 0;
	for (std::string::iterator i = string.begin();
			i != string.end(); i++) {
		uint32 cycleNum = cycle_num_for_char(*i);
		Bitmap* newFrame = fontRes->FrameForCycle(cycleNum, 0);
		totalWidth += newFrame->Frame().w;
		maxHeight = std::max(newFrame->Frame().h, maxHeight);
		frames.push_back(newFrame);
	}
	return frames;
}


/* static */
void
TextSupport::_RenderBitmaps(std::vector<Bitmap*> frames, Bitmap* bitmap,
		GFX::rect rect, uint32 flags)
{
	for (std::vector<Bitmap*>::const_iterator i = frames.begin();
			i != frames.end(); i++) {
		Bitmap* letter = *i;
		
		/*if (flags & IE::LABEL_JUSTIFY_BOTTOM)
			rect.y = destRect.h - letter->Height();
		else if (flags & IE::LABEL_JUSTIFY_TOP)*/
		//	rect.y = 0;
		/*else
			rect.y = (destRect.h - letter->Height()) / 2;*/
		//rect.y += destRect.y;

		rect.w = letter->Frame().w;
		rect.h = letter->Frame().h;
		GraphicsEngine::BlitBitmap(letter, NULL, bitmap, &rect);
		rect.x += letter->Frame().w;
		letter->Release();		
	}
}
