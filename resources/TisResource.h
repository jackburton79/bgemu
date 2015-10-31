#ifndef __TISRESOURCE_H
#define __TISRESOURCE_H

#include "Resource.h"

#include <map>

class Bitmap;
class TISResource : public Resource {
public:
	TISResource(const res_ref &name);
	
	virtual bool Load(Archive *archive, uint32 key);

	Bitmap* TileAt(int index);

private:
	virtual ~TISResource();
	void _Init();
	Bitmap* _GetTileAt(int index);

	uint32 fDataOffset;
	std::map<int, Bitmap*> fCachedTiles;
};

#endif
