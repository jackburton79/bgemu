#ifndef __GRAPHICS_ENGINE_H
#define __GRAPHICS_ENGINE_H

#include "SDL.h"
#include "Bitmap.h"
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
	static void BlitBitmap(Bitmap* bitmap, GFX::rect* source, Bitmap* surface, GFX::rect* dest);
	static void FillRect(Bitmap* bitmap, GFX::rect* rect, uint8 pixelColor);

	void BlitToScreen(Bitmap* bitmap, GFX::rect* source, GFX::rect* dest);

	void SetVideoMode(uint16 x, uint16 y, uint16 depth, uint16 flags);
	void SaveCurrentMode();
	void RestorePreviousMode();

	GFX::rect VideoArea() const;

	void SetWindowCaption(const char* caption);

	SDL_Surface* ScreenSurface();

	void Flip();
		
private:
	SDL_Surface* fScreen;
	SDL_Rect fOldRect;
	uint16 fOldDepth;
};

#endif
