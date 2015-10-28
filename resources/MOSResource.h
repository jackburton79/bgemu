/*
 * MOSResource.h
 *
 *  Created on: 18/lug/2012
 *      Author: stefano
 */

#ifndef MOSRESOURCE_H_
#define MOSRESOURCE_H_

#include "Resource.h"

class MOSResource : public Resource {
public:
	MOSResource(const res_ref &name);

	virtual bool Load(Archive* archive, uint32 key);

	Bitmap* Image();

	Bitmap* TileAt(uint32 index);
private:
	virtual ~MOSResource();

	uint16 fWidth;
	uint16 fHeight;
	uint16 fColumns;
	uint16 fRows;
	uint32 fBlockSize;
	uint32 fPaletteOffset;
	uint32 fTileOffsets;
	uint32 fPixelDataOffset;
};

#endif /* MOSRESOURCE_H_ */
