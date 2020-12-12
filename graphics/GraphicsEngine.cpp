#include <assert.h>
#include <algorithm>
#include <iostream>

#include "Bitmap.h"
#include "GraphicsEngine.h"
#include "Listener.h"
#include "Log.h"

#include <SDL.h>

static GraphicsEngine *sGraphicsEngine = NULL;

GraphicsEngine::GraphicsEngine()
	:
	fSDLWindow(NULL),
	fSDLRenderer(NULL),
	fSDLTexture(NULL),
	fScreen(NULL),
	fFlags(0),
	fOldDepth(0),
	fOldFlags(0)
{
	fRenderingOffset.x = fRenderingOffset.y = 0;
	fOldRect.w = fOldRect.h = fOldRect.x = fOldRect.y = 0;
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
		throw std::runtime_error("GraphicsEngine: SDL Error");
	SDL_ShowCursor(0);
}


GraphicsEngine::~GraphicsEngine()
{
	fScreen->Release();
	SDL_Quit();
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
	std::cout << "Initializing Graphics Engine... ";
	std::flush(std::cout);
	try {
		sGraphicsEngine = new GraphicsEngine();
	} catch (...) {
		std::cout << RED("Failed!") << std::endl;
		sGraphicsEngine = NULL;
		return false;
	}

	std::cout << GREEN("OK!") << std::endl;
	return true;
}


/* static */
void
GraphicsEngine::Destroy()
{
	std::cout << "GraphicsEngine::Destroy()" << std::endl;
	delete sGraphicsEngine;
}


void
GraphicsEngine::SetClipping(const GFX::rect* rect)
{
	if (rect != NULL) {
		SDL_Rect sdlRect;
		GFXRectToSDLRect(rect, &sdlRect);
		SDL_SetClipRect(fScreen->Surface(), &sdlRect);
	}
	SDL_SetClipRect(fScreen->Surface(), NULL);
}


void
GraphicsEngine::BlitToScreen(const Bitmap* source, GFX::rect *sourceRect,
		GFX::rect *destRect)
{
	SDL_Rect sdlSourceRect;
	SDL_Rect sdlDestRect;
	SDL_Rect* sdlSourceRectPtr = NULL;
	SDL_Rect* sdlDestRectPtr = NULL;
	if (sourceRect != NULL) {
		GFXRectToSDLRect(sourceRect, &sdlSourceRect);
		sdlSourceRectPtr = &sdlSourceRect;
	}
	if (destRect != NULL) {
		GFXRectToSDLRect(destRect, &sdlDestRect);
		sdlDestRectPtr = &sdlDestRect;
	}

	SDL_BlitSurface(source->Surface(), sdlSourceRectPtr,
				fScreen->Surface(), sdlDestRectPtr);

	if (destRect != NULL)
		SDLRectToGFXRect(&sdlDestRect, destRect);
}


/*static*/
void
GraphicsEngine::BlitBitmap(const Bitmap* source, GFX::rect *sourceRect,
		Bitmap *dest, GFX::rect *destRect)
{
	SDL_Rect sdlSourceRect;
	SDL_Rect sdlDestRect;
	SDL_Rect* sdlSourceRectPtr = NULL;
	SDL_Rect* sdlDestRectPtr = NULL;
	if (sourceRect != NULL) {
		GFXRectToSDLRect(sourceRect, &sdlSourceRect);
		sdlSourceRectPtr = &sdlSourceRect;
	}
	if (destRect != NULL) {
		GFXRectToSDLRect(destRect, &sdlDestRect);
		sdlDestRectPtr = &sdlDestRect;
	}

	SDL_BlitSurface(source->Surface(), sdlSourceRectPtr,
			dest->Surface(), sdlDestRectPtr);

	if (destRect != NULL)
		SDLRectToGFXRect(&sdlDestRect, destRect);
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


/* static */
void
GraphicsEngine::BlitBitmapScaled(const Bitmap* bitmap, GFX::rect* sourceRect,
							Bitmap* surface, GFX::rect* destRect)
{
	SDL_Rect sdlSourceRect;
	SDL_Rect sdlDestRect;
	SDL_Rect* sdlSourceRectPtr = NULL;
	SDL_Rect* sdlDestRectPtr = NULL;
	if (sourceRect != NULL) {
		GFXRectToSDLRect(sourceRect, &sdlSourceRect);
		sdlSourceRectPtr = &sdlSourceRect;
	}
	if (destRect != NULL) {
		GFXRectToSDLRect(destRect, &sdlDestRect);
		sdlDestRectPtr = &sdlDestRect;
	}
	SDL_SoftStretch(bitmap->Surface(), sdlSourceRectPtr,
			surface->Surface(), sdlDestRectPtr);
}


void
GraphicsEngine::SetVideoMode(uint16 width, uint16 height, uint16 depth,
		uint16 flags)
{
	std::cout << "GraphicsEngine::SetVideoMode(";
	std::cout << std::dec << width << ", " << height << ", " << depth;
	std::cout << ")" << std::endl;

	int SDLWindowFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
	if (flags & VIDEOMODE_FULLSCREEN)
		SDLWindowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	if (SDL_CreateWindowAndRenderer(width, height, SDLWindowFlags,
			&fSDLWindow, &fSDLRenderer) != 0) {
		throw std::runtime_error("Cannot Create Window");
	}

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");  // make the scaled rendering look smoother.
	SDL_RenderSetLogicalSize(fSDLRenderer, width, height);

	SDL_Surface* surface = SDL_CreateRGBSurface(0, width, height, 32,
	                                        0, 0, 0, 0);
	fSDLTexture = SDL_CreateTexture(fSDLRenderer, SDL_PIXELFORMAT_RGB888,
						SDL_TEXTUREACCESS_STREAMING,
						width, height);

	fScreen = new Bitmap(surface, false);
	fFlags = flags;

	std::cout << "GraphicsEngine::SetVideoMode(): Got mode ";
	std::cout << fScreen->Width() << "x";
	std::cout << fScreen->Height() << "x" << fScreen->BitsPerPixel();
	std::cout << std::endl;

	std::vector<Listener*>::iterator i;
	for (i = fListeners.begin(); i != fListeners.end(); i++) {
		(*i)->VideoModeChanged(width, height, depth);
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
GraphicsEngine::ScreenFrame() const
{
	return fScreen->Frame();
}


void
GraphicsEngine::SetRenderingOffset(const GFX::point& point)
{
	fRenderingOffset = point;
}


void
GraphicsEngine::SetWindowCaption(const char* caption)
{
	SDL_SetWindowTitle(fSDLWindow, caption);
}


Bitmap*
GraphicsEngine::ScreenBitmap()
{
	return fScreen;
}


void
GraphicsEngine::Flip()
{
	SDL_UpdateTexture(fSDLTexture, NULL,
			fScreen->Surface()->pixels,
			fScreen->Surface()->pitch);
	SDL_RenderClear(fSDLRenderer);
	SDL_Rect rect = { 0, 0, ScreenFrame().w, ScreenFrame().h };
	rect.x = fRenderingOffset.x;
	rect.y = fRenderingOffset.y;
	SDL_RenderCopy(fSDLRenderer, fSDLTexture, NULL, &rect);
	SDL_RenderPresent(fSDLRenderer);
}


void
GraphicsEngine::SetFade(uint16 value)
{
	if (value <= 255)
		SDL_SetTextureColorMod(fSDLTexture, value, value, value);
}
