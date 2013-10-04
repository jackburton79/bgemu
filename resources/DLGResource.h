/*
 * DLGResource.h
 *
 *  Created on: 04/ott/2013
 *      Author: stefano
 */

#ifndef DLGRESOURCE_H_
#define DLGRESOURCE_H_

#include "Resource.h"

class DLGResource : public Resource {
public:
	DLGResource(const res_ref& name);
	~DLGResource();
};

#endif /* DLGRESOURCE_H_ */
