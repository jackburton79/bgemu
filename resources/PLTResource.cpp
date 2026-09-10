/*
 * PLTResource.cpp
 */

#include "PLTResource.h"

#include "Bitmap.h"
#include "BmpResource.h"
#include "CreResource.h"
#include "ResManager.h"
#include "Stream.h"

#include <iostream>


/* static */
Resource*
PLTResource::Create(const res_ref& name)
{
	return new PLTResource(name);
}


PLTResource::PLTResource(const res_ref& name)
	:
	Resource(name, RES_PLT),
	fWidth(0),
	fHeight(0)
{
}


PLTResource::~PLTResource()
{
}


/* virtual */
bool
PLTResource::Load(Archive* archive, uint32 key)
{
	if (!Resource::Load(archive, key))
		return false;

	fData->ReadAt(0x10, fWidth);
	fData->ReadAt(0x14, fHeight);
	return true;
}


uint32
PLTResource::Width() const
{
	return fWidth;
}


uint32
PLTResource::Height() const
{
	return fHeight;
}


// Body (starting at offset 0x18): width*height pixels, stored left to
// right, bottom to top, each a (intensity, category) byte pair - see
// IESDP plt_v1.htm. intensity 255 marks a fully transparent pixel (left
// untouched, colorkeyed away below).
//
// Reverse-engineered against real game data (this engine's IESDP mirror
// doesn't document MPAL256.BMP's own layout, or which byte is which):
// MPAL256.BMP is 256 columns by 120 rows, one row per colour-gradient id -
// it stands in for the real engine's GetPalette(id). The pixel's
// "category" byte (only 0-7 ever appear in real character dolls) picks
// which CRE colour to recolour that pixel with; the "intensity" byte then
// picks the shade (column) within that gradient.
//
// 0=skin 1=hair 2=metal 3=armor 4=leather 5=minor 6=major
// Category 7 is the outline/shadow edge and has no CRE colour of its own
// MPAL256 column 255 is pure green (0,255,0),
// the format's transparency marker - so intensity 255 pixels are left as
// the transparent clear colour, and green is the colour key (not black,
// which a genuinely dark doll pixel can legitimately be).
Bitmap*
PLTResource::Image(const CREColors& colors) const
{
	BMPResource* mpalResource = gResManager->GetBMP("MPAL256");
	if (mpalResource == NULL) {
		std::cerr << "PLTResource::Image(): couldn't load MPAL256.BMP"
			<< std::endl;
		return NULL;
	}
	Bitmap* mpal = mpalResource->Image();
	gResManager->ReleaseResource(mpalResource);
	if (mpal == NULL)
		return NULL;

	Bitmap* bitmap = new Bitmap((uint16)fWidth, (uint16)fHeight, 16);
	const uint32 transparent = bitmap->MapRGBColor(0, 255, 0);
	bitmap->Clear(transparent);

	const uint8 materialRow[8] = {
		colors.skin, colors.hair, colors.metal, colors.armor,
		colors.leather, colors.minor, colors.major, 0
	};
	const uint16 mpalHeight = mpal->Height();

	const uint8* body = (const uint8*)fData->Data() + 0x18;
	uint32 index = 0;
	for (int32 y = (int32)fHeight - 1; y >= 0; y--) {
		for (uint32 x = 0; x < fWidth; x++) {
			uint8 intensity = body[index * 2];
			uint8 category = body[index * 2 + 1];
			index++;

			if (intensity == 255)
				continue;

			uint8 material = category & 0x7f;
			if (material > 7)
				material = 7;
			uint16 mpalRow = materialRow[material];
			if (mpalRow >= mpalHeight)
				mpalRow = mpalHeight - 1;

			uint8 r, g, b;
			mpal->GetRGBColor(mpal->GetPixel(intensity, mpalRow), r, g, b);
			uint32 pixel = bitmap->MapRGBColor(r, g, b);
			if (pixel == transparent) // keep a real green-ish pixel visible
				pixel = bitmap->MapRGBColor(r, g > 0 ? g - 1 : 0, b);
			bitmap->PutPixel(x, y, pixel);
		}
	}

	mpal->Release();

	bitmap->SetColorKey(0, 255, 0, true);
	return bitmap;
}
