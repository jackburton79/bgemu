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
// MPAL256.BMP is 256 columns by 120 rows. A pixel's "category" byte,
// modulo 128 (values 128-255 mirror 0-127, per the doc), selects the row:
// 0=skin, 1=hair, 2=metal, 3=leather, 4=armor, 5=minor colour, 6=major
// colour (mirroring CREColors) are recolored per-creature - substitute
// in the creature's own color id for that material instead of using the
// category value as the row directly; every other category (7-127) is a
// fixed shading/outline gradient, same for every creature, so it's used
// as the row as-is. The "intensity" byte then selects the column (shade)
// within whichever row was chosen. Verified by rendering real paperdolls
// (CHMF4INV/CHFT1INV with ANOMEN10/Imoen's actual CRE colors) and
// confirming the result is a coherent, correctly colored character doll,
// not the IESDP text - this compositing algorithm isn't documented there.
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
	bitmap->Clear(bitmap->MapRGBColor(0, 0, 0));

	const uint8 materialRow[7] = {
		colors.skin, colors.hair, colors.metal, colors.leather,
		colors.armor, colors.minor, colors.major
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
			uint16 mpalRow = material < 7 ? materialRow[material] : material;
			if (mpalRow >= mpalHeight)
				mpalRow = mpalHeight - 1;

			uint8 r, g, b;
			mpal->GetRGBColor(mpal->GetPixel(intensity, mpalRow), r, g, b);
			bitmap->PutPixel(x, y, bitmap->MapRGBColor(r, g, b));
		}
	}

	mpal->Release();

	bitmap->SetColorKey(0, 0, 0, true);
	return bitmap;
}
