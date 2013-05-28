#include "FileArchive.h"
#include "FileStream.h"
#include "MemoryStream.h"


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
MemoryStream*
PlainFileArchive::ReadResource(res_ref& name, const uint32& key,
		const uint16& type)
{
	// Ignore name, key and type, since the file
	// contains only one resource
	const uint32 size = fFile->Size();
	MemoryStream *stream = new MemoryStream(size);
	ssize_t sizeRead = fFile->ReadAt(0, stream->Data(), size);
	if (sizeRead < 0 || (size_t)sizeRead != size) {
		delete stream;
		return NULL;
	}

	return stream;
}

