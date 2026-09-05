#include "Graphics.h"
#include "GraphicsEngine.h"

#include <assert.h>
#include <string.h>

/* static */
int
Graphics::DecodeRLE(const void* source, uint32 outSize, void* dest,
					uint8 compIndex)
{
	uint8* bits = (uint8*) dest;
	const uint8* srcBits = (const uint8*) source;
	uint32 written = 0;
	while (written < outSize) {
		uint8 byte = *srcBits++;
		if (byte == compIndex) {
			uint16 howMany = (uint8) *srcBits++;
			howMany++;
			// Clamp: a corrupted/malicious run length must never write
			// past the end of the (fixed-size) dest buffer, regardless of
			// what the compressed data claims.
			if (howMany > outSize - written)
				howMany = outSize - written;
			memset(bits, byte, howMany);
			bits += howMany;
			written += howMany;
		} else {
			*bits++ = byte;
			written++;
		}
	}

	return written;
}

/* static */
void
Graphics::ApplyShade(Bitmap* bitmap)
{
	GFX::Palette palette;
	bitmap->GetPalette(palette);
	for (int i = 0; i < 256; i++) {
		uint8 r = palette.colors[i].r;
		uint8 g = palette.colors[i].g;
		uint8 b = palette.colors[i].b;
		uint32 test = (r + g + b) / 3;
		if (test > 2) {
			if (r == 0 && g == 255 && b == 0)
				palette.colors[i].a = 255;
			else
				palette.colors[i].a = std::min(int(test * 2), 255);
		} else
			palette.colors[i].a = 0;
	}
	bitmap->SetAlpha(128, true);
	bitmap->SetPalette(palette);
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

