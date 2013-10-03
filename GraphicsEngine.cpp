#include <assert.h>
#include <algorithm>
#include <iostream>

#include "Bitmap.h"
#include "Graphics.h"
#include "GraphicsEngine.h"
#include "Listener.h"

#include <SDL.h>

static GraphicsEngine *sGraphicsEngine = NULL;

GraphicsEngine::GraphicsEngine()
	:
	fScreen(NULL),
	fOldDepth(0)
{
	fOldRect.w = fOldRect.h = fOldRect.x = fOldRect.y = 0;
	SDL_Init(SDL_INIT_VIDEO);
	SDL_ShowCursor(0);
}


GraphicsEngine::~GraphicsEngine()
{
	SDL_Quit();
	delete fScreen;
}


/* static */
GraphicsEngine*
GraphicsEngine::Get()
{
	return sGraphicsEngine;
}


/* static */
bool
GraphicsEngine::Initialize()
{
	try {
		sGraphicsEngine = new GraphicsEngine();
	} catch (...) {
		return false;
	}

	return true;
}


/* static */
void
GraphicsEngine::Destroy()
{
	delete sGraphicsEngine;
	SDL_Quit();
}


/* static */
Bitmap*
GraphicsEngine::CreateBitmap(uint16 width, uint16 height, uint16 depth)
{
	Bitmap* bitmap = new Bitmap(width, height, depth);
	bitmap->Acquire();

	return bitmap;
}


/* static */
void
GraphicsEngine::DeleteBitmap(Bitmap* bitmap)
{
	if (bitmap != NULL && bitmap->Release()) {
		delete bitmap;
	}
}


void
GraphicsEngine::SetClipping(const GFX::rect* rect)
{
	SDL_SetClipRect(fScreen->Surface(), (const SDL_Rect*)rect);
}


void
GraphicsEngine::BlitToScreen(const Bitmap* bitmap, GFX::rect *source,
		GFX::rect *dest)
{
	SDL_BlitSurface(bitmap->Surface(), (SDL_Rect*)source,
			fScreen->Surface(), (SDL_Rect*)dest);
}


/*static*/
void
GraphicsEngine::BlitBitmap(const Bitmap* bitmap, GFX::rect *source,
		Bitmap *surface, GFX::rect *dest)
{
	SDL_BlitSurface(bitmap->Surface(), (SDL_Rect*)(source),
			surface->Surface(), (SDL_Rect*)(dest));
}


/*static*/
void
GraphicsEngine::BlitBitmapWithMask(const Bitmap* bitmap,
		GFX::rect *source, Bitmap *destBitmap, GFX::rect *dest,
		const Bitmap* mask, GFX::rect *maskRect)
{
	uint32 xStart = 0;
	uint32 yStart = 0;
	if (dest->x < 0) {
		xStart -= dest->x;
		dest->x = 0;
	}
	if (dest->y < 0) {
		yStart -= dest->y;
		dest->y = 0;
	}

	mask->Lock();

	uint8* maskPixels = (uint8*)mask->Pixels()
				+ (maskRect->y * mask->Pitch()) + maskRect->x;

	for (uint32 y = yStart; y < bitmap->Height(); y++) {
		for (uint32 x = xStart; x < bitmap->Width(); x++) {
			SDL_Rect sourceRect = {0, 0, 1, 1};
			SDL_Rect destRect = {0, 0, 1, 1};
			if (maskPixels[x] != MASK_COMPLETELY) {
				if (maskPixels[x] == MASK_SHADE) {
					if (y % 2 != 0 || x % 2 != 0)
						continue;
				}
				sourceRect.x = x;
				sourceRect.y = y;
				destRect.x = x - xStart + dest->x;
				destRect.y = y - yStart + dest->y;

				SDL_BlitSurface(bitmap->Surface(), &sourceRect,
						destBitmap->Surface(), &destRect);
			}
		}
		maskPixels += mask->Pitch();
	}
	mask->Unlock();
}


/*static*/
void
GraphicsEngine::FillRect(Bitmap* bitmap, GFX::rect* rect, uint8 pixelColor)
{
	SDL_FillRect(bitmap->Surface(), (SDL_Rect*)rect, (Uint8)pixelColor);
}


void
GraphicsEngine::FillRect(const GFX::rect& rect, uint32 color)
{
	SDL_Rect sdlRect = { rect.x, rect.y, rect.w, rect.h };
	SDL_FillRect(fScreen->Surface(), &sdlRect, color);
}


void
GraphicsEngine::SetVideoMode(uint16 x, uint16 y, uint16 depth,
		uint16 flags)
{
	int sdlFlags = 0;
	if (flags & VIDEOMODE_FULLSCREEN)
		sdlFlags |= SDL_FULLSCREEN;
	SDL_Surface* surface = SDL_SetVideoMode(x, y, depth, sdlFlags);
	fScreen = new Bitmap(surface, false);
	fFlags = flags;

	std::vector<Listener*>::iterator i;
	for (i = fListeners.begin(); i != fListeners.end(); i++) {
		(*i)->VideoAreaChanged(x, y);
	}
}


void
GraphicsEngine::SaveCurrentMode()
{
	if (fScreen != NULL) {
		SDL_Surface* surface = fScreen->Surface();
		fOldRect.x = fOldRect.y = 0;
		fOldRect.w = surface->w;
		fOldRect.h = surface->h;
		fOldDepth = surface->format->BitsPerPixel;
		fOldFlags = fFlags;
	}
}


void
GraphicsEngine::RestorePreviousMode()
{
	if (fOldDepth != 0) {
		SetVideoMode(fOldRect.w, fOldRect.h, fOldDepth, fOldFlags);
		fOldDepth = 0;
	}
}


GFX::rect
GraphicsEngine::VideoArea() const
{
	return GFX::rect(0, 0, uint16(fScreen->Surface()->w),
			uint16(fScreen->Surface()->h));
}


void
GraphicsEngine::SetWindowCaption(const char* caption)
{
	SDL_WM_SetCaption(caption, NULL);
}


Bitmap*
GraphicsEngine::ScreenBitmap()
{
	return fScreen;
}


void
GraphicsEngine::Flip()
{
	fScreen->Update();
}


/* static */
void
GraphicsEngine::CreateGradient(const Color& start, const Color& end, Palette& palette)
{
	Color* colors = palette.colors;
	uint8 rFactor = (start.r - end.r) / 255;
	uint8 gFactor = (start.g - end.g) / 255;
	uint8 bFactor = (start.b - end.b) / 255;
	uint8 aFactor = (start.a - end.a) / 255;
	colors[0] = start;
	colors[255] = end;
	for (uint8 c = 1; c < 255; c++) {
		colors[c].r = colors[c - 1].r + rFactor;
		colors[c].g = colors[c - 1].g + gFactor;
		colors[c].b = colors[c - 1].b + bFactor;
		colors[c].a = colors[c - 1].a + aFactor;
	}
}


void
GraphicsEngine::AddListener(Listener* listener)
{
	fListeners.push_back(listener);
}


void
GraphicsEngine::RemoveListener(Listener* listener)
{
	fListeners.erase(std::find(fListeners.begin(), fListeners.end(), listener));
}
