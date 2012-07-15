#include "Archive.h"
#include "AreaResource.h"
#include "BCSResource.h"
#include "BamResource.h"
#include "BmpResource.h"
#include "CreResource.h"
#include "FileStream.h"
#include "IDSResource.h"
#include "KEYResource.h"
#include "MveResource.h"
#include "MemoryStream.h"
#include "Resource.h"
#include "SupportDefs.h"
#include "IETypes.h"
#include "TisResource.h"
#include "TLKResource.h"
#include "WedResource.h"

#include <cstdio>


static bool
is_tileset(int16 type)
{
	return type == RES_TIS;
}


// This function is not thread safe
const char *
strresource(int type)
{
	static char sTemp[256];
	switch (type) {
		case RES_BMP:
			return "Bitmap (BMP) format";
		case RES_ARA:
			return "AREA format";
		case RES_MVE:
			return "Movie (MVE) format";
		case RES_WAV:
			return "WAV format";
		case RES_PLT:
			return "Paper Dolls (PLT) format";
		case RES_ITM:
			return "Item";
		case RES_BAM:
	 		return "BAM format";
	 	case RES_WED:
	 		return "WED format";
	 	case RES_TIS:
	 		return "TIS format";
	 	case RES_MOS:
	 		return "MOS format";
	 	case RES_SPL:
			return "SPL format";
	 	case RES_IDS:
	 		return "IDS format";
		case RES_BCS:
			return "Compiled script (BCS format)";
		case RES_CRE:
		 	return "Creature";
		case RES_DLG:
		 	return "Dialog";
		case RES_EFF:
		 	return "EFF Effect";
		case RES_VVC:
		 	return "VVC Effect";
		case RES_2DA:
			return "2DA format";
		case RES_STO:
			return "STORE format";
		case RES_WMP:
			return "World map format";
		case RES_CHU:
			return "CHU format";
		case RES_GAM:
			return "GAM format";
		case RES_PRO:
			return "PRO format (projectile)";
		case RES_WFX:
			return "WFX format";
		default:
		 	sprintf(sTemp, "unknown (0x%x)", type);
			return sTemp;
	}
}


Resource::Resource(const res_ref &name, const uint16 &type)
	:
	fData(NULL),
	fKey(0),
	fType(type),
	fName(name)
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

	//printf("%s: size: %d\n", (const char *)fName, size);

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
Resource::DumpToFile(const char *fileName)
{
	/*FileStream file(fileName, FileStream::WRITE_ONLY);
	
	char buf[4096];
	size_t read;
	fData->Seek(0, SEEK_SET);
	while ((read = fData->Read(buf, sizeof(buf))) > 0)
		;//file.Write(buf, read);*/
	fData->DumpToFile(fileName);
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
Resource::CheckSignature(const char *signature, bool dontWorry)
{
	char array[5];
	array[4] = '\0';

	if (fData->ReadAt(0, array, 4) != 4)
		return false;

	if (strcmp(array, signature) != 0) {
		if (!dontWorry) {
			printf("invalid signature %s (expected %s)\n",
					array, signature);
		}
		return false;
	}

	return true;
}


bool
Resource::CheckVersion(const char *version, bool dontWorry)
{
	char array[5];
	array[4] = '\0';

	if (fData->ReadAt(4, array, 4) != 4)
		return false;

	if (strcmp(array, version) != 0) {
		if (!dontWorry) {
			printf("invalid version %s (expected %s)\n",
				array, version);
		}
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


/* static */
Resource *
Resource::Create(const res_ref &name, uint16 type)
{
	Resource *res = NULL;
	try {
		switch (type) {
			case RES_ARA:
				res = new ARAResource(name);
				break;
			case RES_BAM:
			case RES_MOS:
				res = new BAMResource(name);
				break;
			case RES_BCS:
				res = new BCSResource(name);
				break;
			case RES_CRE:
				res = new CREResource(name);
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
			case RES_MVE:
				res = new MVEResource(name);
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
