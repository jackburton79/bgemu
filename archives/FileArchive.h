#ifndef __PLAINFILEARCHIVE_H
#define __PLAINFILEARCHIVE_H

#include "Archive.h"
#include "IETypes.h"

class MemoryStream;
class FileStream;
class PlainFileArchive : public Archive {
public:
	PlainFileArchive(const char *path);
	virtual ~PlainFileArchive();

	virtual void EnumEntries();

	virtual MemoryStream* ReadResource(res_ref& name,
			const uint32& key,
			const uint16& type);
private:
	FileStream *fFile;
};

#endif // __PLAINFILEARCHIVE_H
