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
	return Bitmap::Load(fData->Data(), fData->Size());
}


bool
BMPResource::Load(Archive *archive, uint32 key)
{
	return Resource::Load(archive, key);
}
