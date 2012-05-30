#ifndef __GRAPHICS_ENGINE_H
#define __GRAPHICS_ENGINE_H

#include "SDL.h"
#include "IETypes.h"




class Bitmap;
class GraphicsEngine {
public:
	GraphicsEngine();
	~GraphicsEngine();
	
	static GraphicsEngine* Get();
	static void Destroy();

	static Bitmap* CreateBitmap(uint16 width, uint16 height, uint16 depth);
	static void DeleteBitmap(Bitmap* sprite);

	enum MIRROR_FLAGS {
		MIRROR_HORIZONTALLY = 1,
		MIRROR_VERTICALLY = 2
	};

	static void MirrorBitmap(Bitmap* bitmap, int flags);

	void SetVideoMode(uint16 x, uint16 y, uint16 depth, uint16 flags);
	void SetWindowCaption(const char* caption);

	SDL_Surface* ScreenSurface();

	void Blit(SDL_Surface *);
	void Flip();
		
private:
	SDL_Surface* fScreen;
	SDL_Surface *fCurrentFrame;
};

#endif
