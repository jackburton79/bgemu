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

/*
static
bool
match_color(const SDL_Color& color, const uint8& r, const uint8& g,
		const uint8& b, const uint8& a)
{
	return color.r == r && color.g == g && color.b == b;
}
*/

/*
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
*/


