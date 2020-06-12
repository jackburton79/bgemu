/*
 * SPLResource.h
 *
 *  Created on: 12 giu 2020
 *      Author: Stefano Ceccherini
 */

#ifndef SPLRESOURCE_H_
#define SPLRESOURCE_H_

#include "Resource.h"

class SPLResource : public Resource {
public:
	static Resource* Create(const res_ref& name);

	SPLResource(const res_ref &name);
};



#endif // SPLRESOURCE_H_
