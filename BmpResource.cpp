#include "Bitmap.h"
#include "BmpResource.h"
#include "MemoryStream.h"

BMPResource::BMPResource(const res_ref &name)
	:
	Resource(name, RES_BMP)
{
}


BMPResource::~BMPResource()
{

}


Bitmap*
BMPResource::Image()
{
	SDL_RWops *rw = SDL_RWFromMem(fData->Data(), fData->Size());
	if (rw == NULL)
		return NULL;

	SDL_Surface *surface = SDL_LoadBMP_RW(rw, 1);
	if (surface != NULL)
		return new Bitmap(surface);

	return NULL;
}


bool
BMPResource::Load(Archive *archive, uint32 key)
{
	return Resource::Load(archive, key);
}
