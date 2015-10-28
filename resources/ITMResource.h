/*
 * ITMResource.h
 *
 *  Created on: 25/mag/2013
 *      Author: stefano
 */

#ifndef __ITMRESOURCE_H_
#define __ITMRESOURCE_H_

#include "Resource.h"


struct itm_header {
	uint32 name_unidentified;
	uint32 name_identified;
	res_ref replacement_item;
	uint32 flags;
	uint16 type;
	char usability[4];
	char animation[2];

	// TODO: Rest
};


class ITMResource: public Resource {
public:
	ITMResource(const res_ref& name);

	virtual bool Load(Archive *archive, uint32 key);

	uint16 Type() const;

private:
	virtual ~ITMResource();

	itm_header fHeader;
};

#endif /* ITMRESOURCE_H_ */
