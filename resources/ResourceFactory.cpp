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
		resource_creation_func creationFunction = get_resource_create(type);		
		res = creationFunction(name);
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

