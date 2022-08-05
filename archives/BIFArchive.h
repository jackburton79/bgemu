#ifndef __BIFARCHIVE_H
#define __BIFARCHIVE_H

#include "Archive.h"


class Stream;
class BIFArchive : public Archive {
public:
	BIFArchive(const char* fileName);
	virtual ~BIFArchive();

	virtual void EnumEntries();

	virtual MemoryStream* ReadResource(res_ref& name,
			const uint32& key,
			const uint16& type);

private:
	ssize_t ReadAt(uint32 offset,
					void* buffer, uint32 size) const;
	ssize_t _ExtractFileBlock(Stream& source, Stream& dest);

	Stream* fStream;

	uint32 fNumEntries;
	uint32 fNumTilesetEntries;
	uint32 fCatalogOffset;
	uint32 fTileEntriesOffset;
};

#endif // __BIFARCHIVE_H
