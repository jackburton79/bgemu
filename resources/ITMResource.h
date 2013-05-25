/*
 * ITMResource.h
 *
 *  Created on: 25/mag/2013
 *      Author: stefano
 */

#ifndef __ITMRESOURCE_H_
#define __ITMRESOURCE_H_

#include "Resource.h"

class ITMResource: public Resource {
public:
	ITMResource(const res_ref& name);
	~ITMResource();

	virtual bool Load(Archive *archive, uint32 key);
};

#endif /* ITMRESOURCE_H_ */
