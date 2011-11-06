#include "Archive.h"
#include "AreaResource.h"
#include "BamResource.h"
#include "BmpResource.h"
#include "FileStream.h"
#include "KeyFile.h"
#include "MemoryStream.h"
#include "Resource.h"
#include "Types.h"
#include "TisResource.h"
#include "WedResource.h"

#include <SupportDefs.h>
#include <cstdio>


static bool
is_tileset(int16 type)
{
	return type == RES_TIS;
}


Resource::Resource(uint8 *data, uint32 size, uint32 key)
	:
	fData(NULL),
	fKey(key),
	fRefCount(0)
{
	fData = new TMemoryStream(data, size, true);
}


Resource::Resource()
	:
	fData(NULL),
	fKey(0),
	fRefCount(0)
{

}


Resource::~Resource()
{
	delete fData;
}


bool
Resource::Load(TArchive *archive, uint32 key)
{
	delete fData;
	fData = NULL;

	fKey = key;

	uint32 index;
	uint32 size;
	uint32 offset;
	bool isTileset = is_tileset(fType);
	if (!isTileset) {
		index = RES_BIF_FILE_INDEX(fKey);
		resource_info info;
		if (!archive->GetResourceInfo(info, index))
			return false;
		size = info.size;
		offset = info.offset;
	} else {
		index = RES_TILESET_INDEX(fKey);
		tileset_info info;
		if (!archive->GetTilesetInfo(info, index))
			return false;
		size = info.numTiles * info.tileSize;
		offset = info.offset;
	}

	fData = new TMemoryStream(size);
	ssize_t sizeRead = archive->ReadAt(offset, fData->Data(), size);
	if (sizeRead < 0 || (size_t)sizeRead != size) {
		delete fData;
		fData = NULL;
		return false;
	}

	return true;
}


void
Resource::Write(const char *fileName)
{
	/*BFile file(fileName, B_WRITE_ONLY|B_CREATE_FILE);
	
	char buf[1024];
	size_t read;
	while ((read = fMemReader->Read(buf, sizeof(buf))) > 0)
		file.Write(buf, read);*/
}


const uint32
Resource::Key() const
{
	return fKey;
}


const uint16
Resource::Type() const
{
	return fType;
}


ssize_t
Resource::ReadAt(int pos, void *dst, int size)
{
	return fData->ReadAt(pos, dst, size);
}


int32
Resource::Seek(int32 where, int whence)
{
	return fData->Seek(where, whence);
}


int32
Resource::Position() const
{
	return fData->Position();
}


/* static */
Resource *
Resource::Create(const res_ref &name, uint16 type)
{
	Resource *res = NULL;
	try {
		switch (type) {
			/*case RES_CRE:
				res = new BGCreature(resPtr, chunk.size, chunk.key);
				break;*/
			case RES_BAM:
			case RES_MOS:
				res = new BAMResource(name);
				break;
			case RES_BMP:
				res = new BMPResource(name);
				break;
			case RES_TIS:
				res = new TISResource(name);
				break;
			case RES_WED:
				res = new WEDResource(name);
				break;
			case RES_AREA:
				res = new AREAResource(name);
				break;
			default:
				throw "Unknown resource!";
				break;
		}
	} catch (...) {
		printf("Resource::Create(): exception thrown!\n");
	}

	if (res != NULL)
		res->fName = name;

	return res;
}


bool
Resource::ReplaceData(TMemoryStream *stream)
{
	delete fData;
	fData = stream;
	//fData = (uint8*)stream->Raw();

	return true;
}


void
Resource::_Acquire()
{
	fRefCount++;
}


bool
Resource::_Release()
{
	if (--fRefCount <= 0)
		return true;

	return false;
}
