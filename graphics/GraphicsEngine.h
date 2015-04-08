#ifndef __GRAPHICS_ENGINE_H
#define __GRAPHICS_ENGINE_H

#include "Bitmap.h"
#include "GraphicsDefs.h"

#include <vector>

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

	static void BlitBitmap(const Bitmap* bitmap, GFX::rect* source, Bitmap* surface, GFX::rect* dest);
	static void BlitBitmapWithMask(const Bitmap* bitmap, GFX::rect* source,
								Bitmap* surface, GFX::rect* dest,
								const Bitmap* mask, GFX::rect* maskRect);
	
	void SetClipping(const GFX::rect* rect);
	void BlitToScreen(const Bitmap* bitmap, GFX::rect* source, GFX::rect* dest);

	enum VIDEOMODE_FLAGS {
		VIDEOMODE_WINDOWED = 0,
		VIDEOMODE_FULLSCREEN = 1
	};
	void SetVideoMode(uint16 x, uint16 y, uint16 depth, uint16 flags);
	void SaveCurrentMode();
	void RestorePreviousMode();

	GFX::rect ScreenFrame() const;

	void SetWindowCaption(const char* caption);

	Bitmap* ScreenBitmap();

	void Flip();

	static void CreateGradient(const GFX::Color& start,
								const GFX::Color& end,
								GFX::Palette& palette);

private:
	Bitmap* fScreen;
	uint16 fFlags;

	GFX::rect fOldRect;
	uint16 fOldDepth;
	uint16 fOldFlags;

	std::vector<Listener*> fListeners;
	
	GraphicsEngine();
	~GraphicsEngine();

};

#endif
