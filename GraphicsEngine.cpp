#include <assert.h>
#include <iostream>

#include "Bitmap.h"
#include "Graphics.h"
#include "GraphicsEngine.h"




static GraphicsEngine *sGraphicsEngine = NULL;

GraphicsEngine::GraphicsEngine()
	:
	fCurrentFrame(NULL)
{
	SDL_Init(SDL_INIT_VIDEO);	
}


GraphicsEngine::~GraphicsEngine()
{
	SDL_FreeSurface(fCurrentFrame);
	SDL_Quit();
}


/* static */
GraphicsEngine*
GraphicsEngine::Get()
{
	if (sGraphicsEngine == NULL)
		sGraphicsEngine = new GraphicsEngine();
	return sGraphicsEngine;
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
	// TODO: keep track of it ?
	return bitmap;
}


/* static */
void
GraphicsEngine::DeleteBitmap(Bitmap* bitmap)
{
	delete bitmap;
}


/* static */
void
GraphicsEngine::MirrorBitmap(Bitmap* bitmap, int flags)
{
	if (flags & MIRROR_VERTICALLY)
		printf("MIRROR_VERTICALLY NOT IMPLEMENTED!!\n");

	if (flags & MIRROR_HORIZONTALLY)
		Graphics::MirrorSDLSurface(bitmap->Surface());
}


void
GraphicsEngine::BlitBitmap(Bitmap* bitmap, GFX::rect *source, Bitmap *surface, GFX::rect *dest)
{
	SDL_BlitSurface(bitmap->Surface(), (SDL_Rect*)source, surface->Surface(), (SDL_Rect*)dest);
}


void
GraphicsEngine::BlitToScreen(Bitmap* bitmap, GFX::rect *source, GFX::rect *dest)
{
	SDL_BlitSurface(bitmap->Surface(), (SDL_Rect*)source, fScreen, (SDL_Rect*)dest);
}


void
GraphicsEngine::SetVideoMode(uint16 x, uint16 y, uint16 depth, uint16 flags)
{
	fScreen = SDL_SetVideoMode(x, y, depth, 0);
}


GFX::rect
GraphicsEngine::VideoArea() const
{
	GFX::rect rect = { 0, 0, fScreen->w, fScreen->h };
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
GraphicsEngine::Blit(SDL_Surface *_surface)
{
	SDL_Surface *surface = SDL_DisplayFormat(_surface);
	SDL_BlitSurface(surface, NULL, fScreen, NULL);
	Flip();
	SDL_FreeSurface(surface);
}
