#include "2DAResource.h"
#include "Archive.h"
#include "AreaResource.h"
#include "BCSResource.h"
#include "BamResource.h"
#include "BmpResource.h"
#include "CHUIResource.h"
#include "CreResource.h"
#include "DLGResource.h"
#include "FileStream.h"
#include "IDSResource.h"
#include "ITMResource.h"
#include "KEYResource.h"
#include "MOSResource.h"
#include "MveResource.h"
#include "MemoryStream.h"
#include "Resource.h"
#include "SupportDefs.h"
#include "IETypes.h"
#include "TisResource.h"
#include "TLKResource.h"
#include "VVCResource.h"
#include "WAVResource.h"
#include "WedResource.h"
#include "WMAPResource.h"

#include <cstdio>


// This function is not thread safe
const char *
strresource(int type)
{
	static char sTemp[64];
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
	Referenceable(0),
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
	fKey = key;

	delete fData;
	fData = archive->ReadResource(fName, key, fType);
	if (fData == NULL)
		return false;

	return true;
}


/* virtual */
Resource*
Resource::Clone()
{
	Resource* newResource = Resource::Create(Name(), Type());
	newResource->fKey = fKey;
	newResource->fData = fData->Clone();
	newResource->Acquire();

	return newResource;
}


void
Resource::Dump()
{
	try {
		if (fData != NULL)
			fData->Dump();
	} catch (const char* string) {
		std::cerr << "Resource::Dump() caught exception: " << string << std::endl;
	} catch (...) {
		std::cerr << "Resource::Dump() caught exception." << std::endl;
	}
}


void
Resource::DumpToFile(const char *fileName)
{
	if (fData != NULL)
		fData->DumpToFile(fileName);
}


uint32
Resource::Key() const
{
	return fKey;
}


uint16
Resource::Type() const
{
	return fType;
}


const char*
Resource::Name() const
{
	return fName.CString();
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
			//printf("invalid signature %s (expected %s)\n",
				//	array, signature);
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
Resource::ReplaceData(Stream *stream)
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


bool
Resource::IsEncrypted()
{
	int32 pos = fData->Position();
	fData->Seek(0, SEEK_SET);

	bool encrypted = (fData->ReadByte() == 0xFF)
		&& (fData->ReadByte() == 0xFF);

	fData->Seek(pos, SEEK_SET);

	return encrypted;
}


/* static */
Resource*
Resource::Create(const res_ref &name, const uint16& type)
{
	Resource *res = NULL;
	try {
		switch (type) {
			case RES_2DA:
				res = new TWODAResource(name);
				break;
			case RES_ARA:
				res = new ARAResource(name);
				break;
			case RES_BAM:
				res = new BAMResource(name);
				break;
			case RES_BCS:
				res = new BCSResource(name);
				break;
			case RES_BMP:
				res = new BMPResource(name);
				break;
			case RES_CHU:
				res = new CHUIResource(name);
				break;
			case RES_CRE:
				res = new CREResource(name);
				break;
			case RES_DLG:
				res = new DLGResource(name);
				break;
			case RES_IDS:
				res = new IDSResource(name);
				break;
			case RES_ITM:
				res = new ITMResource(name);
				break;
			case RES_KEY:
				res = new KEYResource(name);
				break;
			case RES_MVE:
				res = new MVEResource(name);
				break;
			case RES_MOS:
				res = new MOSResource(name);
				break;
			case RES_TIS:
				res = new TISResource(name);
				break;
			case RES_VVC:
				res = new VVCResource(name);
				break;
			case RES_WAV:
				res = new WAVResource(name);
				break;			
			case RES_WED:
				res = new WEDResource(name);
				break;
			case RES_WMP:
				res = new WMAPResource(name);
				break;
			default:
				//throw "Unknown resource!";
				break;
		}
	} catch (...) {
		printf("Resource::Create(): exception thrown!\n");
		res = NULL;
	}

	return res;
}


/* static */
Resource*
Resource::Create(const res_ref& name, const uint16& type,
		const uint32& key, Archive* archive)
{
	Resource* resource = Create(name, type);
	if (resource != NULL) {
		if (!resource->Load(archive, key)) {
			delete resource;
			return NULL;
		}
	}

	return resource;
}
