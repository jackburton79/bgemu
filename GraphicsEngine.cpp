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


/* static */
void
GraphicsEngine::MirrorBitmap(Bitmap* bitmap, int flags)
{
	if (flags & MIRROR_VERTICALLY)
		bitmap->Flip();

	if (flags & MIRROR_HORIZONTALLY)
		bitmap->Mirror();
}


void
GraphicsEngine::SetClipping(const GFX::rect* rect)
{
	SDL_SetClipRect(fScreen, (const SDL_Rect*)rect);
}


void
GraphicsEngine::BlitToScreen(Bitmap* bitmap, GFX::rect *source,
		GFX::rect *dest)
{
	SDL_BlitSurface(bitmap->Surface(), (SDL_Rect*)source,
			fScreen, (SDL_Rect*)dest);
}


void
GraphicsEngine::StrokeRect(const GFX::rect& rect, uint32 color)
{
	Bitmap* temp = new Bitmap(fScreen, false);
	if (temp->Lock()) {
		temp->StrokeRect(rect, color);
		temp->Unlock();
	}
	delete temp;
}


/*static*/
void
GraphicsEngine::BlitBitmap(Bitmap* bitmap, GFX::rect *source,
		Bitmap *surface, GFX::rect *dest)
{
	SDL_BlitSurface(bitmap->Surface(), (SDL_Rect*)(source),
			surface->Surface(), (SDL_Rect*)(dest));
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
	SDL_FillRect(fScreen, &sdlRect, color);
}


void
GraphicsEngine::SetVideoMode(uint16 x, uint16 y, uint16 depth,
		uint16 flags)
{
	fScreen = SDL_SetVideoMode(x, y, depth, 0);
	std::vector<Listener*>::iterator i;
	for (i = fListeners.begin(); i != fListeners.end(); i++) {
		(*i)->VideoAreaChanged(x, y);
	}
}


void
GraphicsEngine::SaveCurrentMode()
{
	if (fScreen != NULL) {
		fOldRect.x = fOldRect.y = 0;
		fOldRect.w = fScreen->w;
		fOldRect.h = fScreen->h;
		fOldDepth = fScreen->format->BitsPerPixel;
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
	GFX::rect rect = { 0, 0, uint16(fScreen->w), uint16(fScreen->h) };
	return rect;
}


void
GraphicsEngine::SetWindowCaption(const char* caption)
{
	SDL_WM_SetCaption(caption, NULL);
}


SDL_Surface*
GraphicsEngine::ScreenSurface()
{
	return fScreen;
}


void
GraphicsEngine::Flip()
{
	SDL_Flip(fScreen);
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
