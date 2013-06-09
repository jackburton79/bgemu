#include "Graphics.h"
#include "GraphicsEngine.h"
#include "Polygon.h"
#include "RectUtils.h"
#include "Utils.h"

#include <assert.h>
#include <string.h>

/* static */
int
Graphics::DecodeRLE(const void *source, uint32 outSize,
		void *dest, uint8 compIndex)
{
	uint32 size = 0;
	uint8 *bits = (uint8*)dest;
	uint8 *srcBits = (uint8*)source;
	while (size++ < outSize) {
		uint8 byte = *srcBits++;
		if (byte == compIndex) {
			uint16 howMany = (uint8)*srcBits++;
			size += howMany;
			howMany++;
			memset(bits, byte, howMany);
			bits += howMany;
		} else {
			*bits++ = byte;
		}
	}

	size--;

	return outSize;
}


/* static */
int
Graphics::DataToBitmap(const void *data, int32 width,
		int32 height, Bitmap *bitmap)
{

	return 0;
}


/* static */
static
bool
match_color(const SDL_Color& color, const uint8& r, const uint8& g,
		const uint8& b, const uint8& a)
{
	return color.r == r && color.g == g && color.b == b;
}


/* static */
Bitmap*
Graphics::ApplyMask(Bitmap* bitmap, Bitmap*, uint16 x, uint16 y)
{
	SDL_Surface* surface = bitmap->Surface();
	//uint32 colorKey = surface->format->colorkey;
	//std::cout << "colorkey: " << colorKey << std::endl;
	//SDL_Color* colors = surface->format->palette->colors;

	SDL_LockSurface(surface);
	for (int32 y = 0; y < surface->h; y++) {
		for (int32 x = 0; x < surface->w; x++) {
			uint8* pixel = (uint8*)surface->pixels + y * surface->pitch + x;
			if (*pixel > 200) {
				*pixel = 0;
			}
		}
	}
	SDL_UnlockSurface(surface);
	//SDL_SaveBMP(surface, "test.bmp");

	//throw -1;
	return bitmap;
}


static int
IndexOfColor(const SDL_Color *color, const SDL_Palette *palette)
{
	for (int32 i = 0; i < palette->ncolors; i++) {
		if (color->r == palette->colors[i].r
			&& color->g == palette->colors[i].g
			&& color->b == palette->colors[i].b) {
			return i;
		}
	}
	return -1;
}



