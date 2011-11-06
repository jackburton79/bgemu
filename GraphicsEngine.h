#ifndef __GRAPHICS_ENGINE_H
#define __GRAPHICS_ENGINE_H

#include "SDL.h"
#include "Types.h"

class GraphicsEngine {
public:
	GraphicsEngine();
	~GraphicsEngine();
	
	void SetVideoMode(uint16 x, uint16 y, uint16 flags); 
	void SetPalette(uint16 start, uint16 count, uint8 palette[]);
	void Blit(SDL_Surface *);
	void Flip();
		
private:
	SDL_Surface *fCurrentFrame;
};

extern GraphicsEngine *gGraphicsEngine;

#endif
