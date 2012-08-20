#ifndef __TISRESOURCE_H
#define __TISRESOURCE_H

#include "Resource.h"

class Bitmap;
class TISResource : public Resource {
public:
	TISResource(const res_ref &name);
	virtual ~TISResource();
	
	bool Load(Archive *archive, uint32 key);

	Bitmap *TileAt(int index);

private:
	void _Init();
	uint32 fDataOffset;
};

#endif
