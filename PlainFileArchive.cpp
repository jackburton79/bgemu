#include "FileStream.h"
#include "PlainFileArchive.h"

PlainFileArchive::PlainFileArchive(const char *path)
	:
	fFile(NULL)
{
	fFile = new FileStream(path, FileStream::READ_ONLY,
					FileStream::CASE_INSENSITIVE);
}


/* virtual */
PlainFileArchive::~PlainFileArchive()
{
	delete fFile;
}


/* virtual */
void
PlainFileArchive::EnumEntries()
{

}


/* virtual */
bool
PlainFileArchive::GetResourceInfo(resource_info &info, uint16 index) const
{
	info.key = 0;
	info.offset = 0;
	info.size = fFile->Size();
	info.type = 0;
	info.unk = 0;

	return true;
}


/* virtual */
bool
PlainFileArchive::GetTilesetInfo(tileset_info &info, uint16 index) const
{
	return false;
}


/* virtual */
ssize_t
PlainFileArchive::ReadAt(uint32 offset, void *buffer, uint32 size) const
{
	return fFile->ReadAt(offset, buffer, size);
}

