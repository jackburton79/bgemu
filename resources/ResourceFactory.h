/*
 * ResourceFactory.h
 *
 *  Created on: 09/giu/2020
 *      Author: Stefano Ceccherini
 */

#ifndef __RESOURCEFACTORY_H
#define __RESOURCEFACTORY_H

#include "IETypes.h"

#include <string>

class Archive;
class Resource;
class ResourceFactory {
public:
	ResourceFactory();
	Resource* CreateResource(const res_ref &name, const uint16& type);
	Resource* CreateResource(const res_ref& name, const uint16& type,
		const uint32& key, Archive* archive);
};

#endif // __RESOURCEFACTORY_H
