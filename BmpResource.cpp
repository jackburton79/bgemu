#include "BmpResource.h"
#include "MemoryStream.h"

BMPResource::BMPResource(const res_ref &name)
	:
	Resource()
{

}


BMPResource::~BMPResource()
{

}


SDL_Surface *
BMPResource::Image()
{
	SDL_RWops *rw = SDL_RWFromMem(fData->Data(), fData->Size());
	if (rw == NULL)
		return NULL;

	return SDL_LoadBMP_RW(rw, 1);
}


bool
BMPResource::Load(TArchive *archive, uint32 key)
{
	return Resource::Load(archive, key);
}
