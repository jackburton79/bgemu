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

	virtual bool Load(Archive *archive, uint32 key);

	uint32 SpellNameUnidentified() const;
	uint32 SpellNameIdentified() const;

	uint32 Flags() const;

	uint16 CastingGraphics() const;

	uint32 SpellDescriptionUnidentified() const;
	uint32 SpellDescriptionIdentified() const;

	uint16 CastingTime() const;

	static std::string GetSpellResourceName(uint16 id);

private:
	uint32 fExtendedHeadersOffset;
	uint16 fExtendedHeadersCount;
};



#endif // SPLRESOURCE_H_
