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
GraphicsEngine::BlitBitmapWithMask(const Bitmap* bitmap, GFX::rect *source,
		Bitmap *destBitmap, GFX::rect *dest, const Bitmap* mask, GFX::rect *maskRect)
{
	dest->x = std::max(dest->x, (sint16)0);
	dest->y = std::max(dest->y, (sint16)0);

	mask->Lock();

	uint8* maskPixels = (uint8*)mask->Pixels()
				+ (maskRect->y * mask->Pitch()) + maskRect->x;
	SDL_Rect sourceRect = {0, 0, 1, 1};
	SDL_Rect destRect = {0, 0, 1, 1};
	for (uint32 y = 0; y < bitmap->Height(); y++) {
		for (uint32 x = 0; x < bitmap->Width(); x++) {
			if (maskPixels[x] == 0) {
				sourceRect.x = x;
				sourceRect.y = y;
				destRect.x = x + dest->x;
				destRect.y = y + dest->y;
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
	SDL_Surface* surface = SDL_SetVideoMode(x, y, depth, 0);
	fScreen = new Bitmap(surface, false);
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
	}
}


void
GraphicsEngine::RestorePreviousMode()
{
	if (fOldDepth != 0) {
		SetVideoMode(fOldRect.w, fOldRect.h, fOldDepth, 0);
		fOldDepth = 0;
	}
}


GFX::rect
GraphicsEngine::VideoArea() const
{
	GFX::rect rect = { 0, 0,
			uint16(fScreen->Surface()->w),
			uint16(fScreen->Surface()->h) };
	return rect;
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
