#include "Archive.h"
#include "AreaResource.h"
#include "BamResource.h"
#include "BmpResource.h"
#include "CreResource.h"
#include "FileStream.h"
#include "IDSResource.h"
#include "KEYResource.h"
#include "MemoryStream.h"
#include "Resource.h"
#include "SupportDefs.h"
#include "Types.h"
#include "TisResource.h"
#include "TLKResource.h"
#include "WedResource.h"

#include <cstdio>


static bool
is_tileset(int16 type)
{
	return type == RES_TIS;
}


Resource::Resource(const res_ref &name, const uint16 &type)
	:
	fData(NULL),
	fKey(0),
	fType(type),
	fName(name),
	fRefCount(0)
{

}


Resource::~Resource()
{
	delete fData;
}


bool
Resource::Load(Archive *archive, uint32 key)
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

	fData = new MemoryStream(size);
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


bool
Resource::CheckSignature(const char *signature)
{
	char array[5];
	array[4] = '\0';

	if (fData->ReadAt(0, array, 4) != 4)
		return false;

	if (strcmp(array, signature) != 0) {
		printf("invalid signature %s (expected %s)\n",
				array, signature);
		return false;
	}

	return true;
}


bool
Resource::CheckVersion(const char *version)
{
	char array[5];
	array[4] = '\0';

	if (fData->ReadAt(4, array, 4) != 4)
		return false;

	if (strcmp(array, version) != 0) {
		printf("invalid version %s (expected %s)\n",
				array, version);
		return false;
	}

	return true;
}


bool
Resource::ReplaceData(MemoryStream *stream)
{
	delete fData;
	fData = stream;

	return true;
}


void
Resource::DropData()
{
	ReplaceData(NULL);
}


// Private
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


/* static */
Resource *
Resource::Create(const res_ref &name, uint16 type)
{
	Resource *res = NULL;
	try {
		switch (type) {
			case RES_CRE:
				res = new CREResource(name);
				break;
			case RES_BAM:
			case RES_MOS:
				res = new BAMResource(name);
				break;
			case RES_BMP:
				res = new BMPResource(name);
				break;
			case RES_IDS:
				res = new IDSResource(name);
				break;
			case RES_TIS:
				res = new TISResource(name);
				break;
			case RES_WED:
				res = new WEDResource(name);
				break;
			case RES_ARA:
				res = new ARAResource(name);
				break;
			default:
				throw "Unknown resource!";
				break;
		}
	} catch (...) {
		printf("Resource::Create(): exception thrown!\n");
	}

	return res;
}
