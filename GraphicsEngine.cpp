#include <assert.h>
#include <iostream>

#include "GraphicsEngine.h"
#include "Stream.h"

GraphicsEngine *gGraphicsEngine = NULL;

GraphicsEngine::GraphicsEngine()
	:
	fCurrentFrame(NULL)
{
	assert(gGraphicsEngine == NULL);
	gGraphicsEngine = this;
	SDL_Init(SDL_INIT_VIDEO);	
}


GraphicsEngine::~GraphicsEngine()
{
	SDL_FreeSurface(fCurrentFrame);
	SDL_Quit();
}


void
GraphicsEngine::SetVideoMode(uint16 x, uint16 y, uint16 flags)
{
	std::cout << "Set video mode:" << std::endl;
	std::cout << std::dec << x << " " << y << " " << flags << std::endl;
	SDL_SetVideoMode(x, y, 16, 0);
}


void
GraphicsEngine::SetPalette(uint16 start, uint16 count, uint8 palette[])
{
//	cout << "Set palette:" << endl;
//	cout << "start: " << start << " count: " << count << endl;
}


void
GraphicsEngine::Flip()
{
	std::cout << "Flip()" << std::endl;
}


void
GraphicsEngine::Blit(SDL_Surface *_surface)
{
	SDL_Surface *surface = SDL_DisplayFormat(_surface);
	SDL_BlitSurface(surface, NULL, SDL_GetVideoSurface(), NULL);
	SDL_Flip(SDL_GetVideoSurface());
	SDL_FreeSurface(surface);
}
