#ifndef __BGCHARACHTER_H
#define __BGCHARACHTER_H

#include "BGCreature.h"

#define CHR_SIGNATURE "CHR "
#define CHR_NAME_LENGTH 32

class BGCharachter : public BGCreature {
public:
	BGCharachter();
	virtual void Load(const char *file);

private:
	char fName[CHR_NAME_LENGTH];
	int32 fCREOffset;
	int32 fCRELength;	
};

#endif

