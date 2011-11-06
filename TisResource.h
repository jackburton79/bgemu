#ifndef __TISRESOURCE_H
#define __TISRESOURCE_H

#include "Resource.h"

#include <SDL.h>

class TISResource : public Resource {
public:
	TISResource(uint8 *data, uint32 size, uint32 key);
	TISResource(const res_ref &name);

	~TISResource();
	
	bool Load(TArchive *archive, uint32 key);

	SDL_Surface *TileCellAt(int index);

private:
	void _Init();

	SDL_Color *fPalette;
};


#endif
