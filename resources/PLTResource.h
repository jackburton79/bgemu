/*
 * PLTResource.h
 *
 * Reads the PLT (paperdoll) format - see IESDP plt_v1.htm.
 *
 * A PLT is a recolorable inventory-screen doll image: each pixel stores a
 * (intensity, category) byte pair instead of a direct color, so the same
 * file renders differently for every creature depending on its CRE color
 * fields (see CREColors, resources/CreResource.h) - the same "metal/
 * minor/major/skin/leather/armor/hair" categories the sprite-coloring
 * system (game/ColorRange.h) already applies to BAM sprites, just via a
 * different lookup table (MPAL256.BMP instead of RANGES12.BMP). See
 * Image()'s own comment for exactly how the two bytes combine.
 */

#ifndef PLTRESOURCE_H_
#define PLTRESOURCE_H_

#include "Resource.h"

class Bitmap;
struct CREColors;

class PLTResource : public Resource {
public:
	PLTResource(const res_ref& name);
	virtual ~PLTResource();

	static Resource* Create(const res_ref& name);

	virtual bool Load(Archive* archive, uint32 key);

	uint32 Width() const;
	uint32 Height() const;

	// Renders this layer into a new Bitmap using the given creature's
	// colors, background transparent (colorkeyed). Caller owns the
	// returned Bitmap (Release() it) - NULL if MPAL256.BMP (the shared
	// color lookup bitmap every PLT depends on) couldn't be loaded.
	Bitmap* Image(const CREColors& colors) const;

private:
	uint32 fWidth;
	uint32 fHeight;
};

#endif // PLTRESOURCE_H_
