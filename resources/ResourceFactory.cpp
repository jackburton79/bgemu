/*
 * ResourceFactory.cpp
 *
 *  Created on: 09/giu/2020
 *      Author: Stefano Ceccherini
 */

#include "ResourceFactory.h"

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


ResourceFactory::ResourceFactory()
{
}


/* static */
Resource*
ResourceFactory::CreateResource(const res_ref &name, const uint16& type)
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
ResourceFactory::CreateResource(const res_ref& name, const uint16& type,
		const uint32& key, Archive* archive)
{
	Resource* resource = CreateResource(name, type);
	if (resource != NULL) {
		if (!resource->Load(archive, key)) {
			delete resource;
			return NULL;
		}
	}

	return resource;
}

