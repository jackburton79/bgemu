#ifndef __GRAPHICS_ENGINE_H
#define __GRAPHICS_ENGINE_H

#include "Bitmap.h"
#include "GraphicsDefs.h"

#include <vector>
#include "SDL.h"
class Bitmap;
class Listener;
class GraphicsEngine {
public:
	static GraphicsEngine* Get();
	static bool Initialize();
	static void Destroy();


	enum MASK_VALUES {
		MASK_NO_MASK = 0,
		MASK_SHADE = 1,
		MASK_COMPLETELY = 255
	};

	static bool BlitBitmap(const Bitmap* sourceBitmap, GFX::rect* sourceRect,
						   Bitmap* destBitmap, GFX::rect* destRect);
	static bool BlitBitmapWithMask(const Bitmap* sourceBitmap, GFX::rect* sourceRect,
								Bitmap* destBitmap, GFX::rect* destRect,
								const Bitmap* mask, GFX::rect* maskRect);
	static bool BlitBitmapScaled(const Bitmap* sourceBitmap, GFX::rect* sourceRect,
							Bitmap* destBitmap, GFX::rect* destRect);

	enum BLITTING_FLAGS {
		BLITTING_NORMAL = 0,
		BLITTING_SCALED = 1
	};
	bool BlitToScreen(const Bitmap* sourceBitmap,
						  const GFX::point& position);
	bool BlitToScreen(const Bitmap* sourceBitmap,
					  GFX::rect* sourceRect,
					  GFX::rect* destRect);
	bool BlitToScreenScaled(const Bitmap* sourceBitmap,
					  GFX::rect* sourceRect,
					  GFX::rect* destRect);

	void SetClipping(const GFX::rect* rect);

	enum VIDEOMODE_FLAGS {
		VIDEOMODE_WINDOWED = 0,
		VIDEOMODE_FULLSCREEN = 1
	};
	void SetVideoMode(uint16 x, uint16 y, uint16 depth, uint16 flags);
	void SaveCurrentMode();
	void RestorePreviousMode();

	GFX::rect ScreenFrame() const;
	void SetRenderingOffset(const GFX::point& point);

	void SetWindowCaption(const char* caption);

	Bitmap* ScreenBitmap();	
	void Update();
	
	void SetFade(uint16 value);

private:
	SDL_Window *fSDLWindow;
	SDL_Renderer *fSDLRenderer;
	SDL_Texture* fSDLTexture;
	Bitmap* fScreen;
	uint16 fFlags;
	GFX::point fRenderingOffset;

	GFX::rect fOldRect;
	uint16 fOldDepth;
	uint16 fOldFlags;

	std::vector<Listener*> fListeners;
	
	GraphicsEngine();
	~GraphicsEngine();
};

#endif
