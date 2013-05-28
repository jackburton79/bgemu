#include <assert.h>
#include <iostream>
#include <string.h>

#include "BIFArchive.h"
#include "FileStream.h"
#include "MemoryStream.h"

#include "zlib.h"

#define BIF_SIGNATURE "BIFFV1  "
#define BIFC_SIGNATURE "BIFCV1.0"


using namespace std;

BIFArchive::BIFArchive(const char *fileName)
	:
	fStream(NULL)
{
	fStream = new FileStream(fileName, FileStream::READ_ONLY,
					FileStream::CASE_INSENSITIVE);
	
	char signature[9];
	signature[8] = '\0';
	fStream->Read(signature, 8);
		
	if (!strcmp(signature, BIFC_SIGNATURE)) {
		//cout << "BIFC archive, decompressing... ";
		FileStream *tmpStream = dynamic_cast<FileStream*>(fStream);

		uint32 uncompressedSize;
		tmpStream->Read(&uncompressedSize, sizeof(uncompressedSize));
		
		fStream = new MemoryStream(uncompressedSize);
		uint32 done = 0;
		do {
			ssize_t blockSize = _ExtractFileBlock(*tmpStream, *fStream);
			if (blockSize < 0)
				break;
			done += blockSize;
		} while (done < uncompressedSize);
			
		delete tmpStream;
		
		if (done != uncompressedSize) {
			delete fStream;
			throw -1;
		}

		fStream->Seek(0, SEEK_SET);
		fStream->Read(signature, 8);
	}

	if (strcmp(signature, BIF_SIGNATURE)) {
		cout << "Unknown archive" << endl;
		throw -1;
	}
 			
	
	(*fStream) >> fNumEntries >> fNumTilesetEntries >> fCatalogOffset;
	fTileEntriesOffset = fCatalogOffset + fNumEntries * sizeof(resource_info);
}


BIFArchive::~BIFArchive()
{
	delete fStream;
}


void
BIFArchive::EnumEntries()
{
	fStream->Seek(fCatalogOffset, SEEK_SET);
	
	uint32 locator;
	uint32 offset;
	uint32 size;
	uint16 type;
	uint16 unk;
	uint32 tileSize;
	uint32 numTiles;
	
	char name[9];
	name[8] = '\0';
		
	for (uint32 c = 0; c < fNumEntries; c++) {
		(*fStream) >> locator >> offset >> size >> type >> unk;
		cout << "Resource " << dec << c << " : size = " << size;
		cout << ", offset = " << offset << ", type: ";
		cout << strresource(type) << "(0x" << hex << type << ")";
		
		fStream->ReadAt(offset, name, 8);
		
		cout << " name : " << name << endl << dec; 
	}
	
	fStream->Seek(fTileEntriesOffset, SEEK_SET);
	for (uint32 c = 0; c < fNumTilesetEntries; c++) {
		(*fStream) >> locator >> offset >> numTiles >> tileSize >> type >> unk;
		cout << "Resource " << dec << c << " : numTiles = " << numTiles;
		cout << ", offset = " << offset << ", type: ";
		cout << strresource(type);
		
		fStream->ReadAt(offset, name, 8);
		
		cout << " name : " << name << endl << dec; 
	}
}


/* virtual */
MemoryStream*
BIFArchive::ReadResource(res_ref& name, const uint32& key,
		const uint16& type)
{
	uint32 index;
	uint32 size;
	uint32 offset;
	bool isTileset = is_tileset(type);
	if (!isTileset) {
		index = RES_BIF_FILE_INDEX(key);
		resource_info info;
		if (!_GetResourceInfo(info, index))
			return false;
		size = info.size;
		offset = info.offset;
	} else {
		index = RES_TILESET_INDEX(key);
		tileset_info info;
		if (!_GetTilesetInfo(info, index))
			return false;
		size = info.numTiles * info.tileSize;
		offset = info.offset;
	}

	MemoryStream *stream = new MemoryStream(size);
	ssize_t sizeRead = fStream->ReadAt(offset, stream->Data(), size);
	if (sizeRead < 0 || (size_t)sizeRead != size) {
		delete stream;
		return NULL;
	}

	return stream;
}


bool
BIFArchive::_GetResourceInfo(resource_info &info, uint16 index) const
{
	fStream->ReadAt(fCatalogOffset + index * sizeof(resource_info),
			&info, sizeof(resource_info));
	return true;
}


bool
BIFArchive::_GetTilesetInfo(tileset_info &info, uint16 index) const
{
	fStream->ReadAt(fTileEntriesOffset + index * sizeof(tileset_info),
			&info, sizeof(tileset_info));
	return true;
}


ssize_t
BIFArchive::_ExtractFileBlock(Stream &source, Stream &dest)
{
	uint32 decomp;
	uint32 comp;
	source.Read(&decomp, sizeof(decomp));
	source.Read(&comp, sizeof(comp));

	uint8 *buffer = NULL;
	uint8 *destBuffer = NULL;
	try {
		buffer = new uint8[comp];
		destBuffer = new uint8[decomp];

		source.Read(buffer, comp);
		int status = uncompress((Bytef*)destBuffer, (uLongf*)&decomp,
				(const Bytef*)buffer, (uLong)comp);

		if (status == Z_OK)
			dest.Write(destBuffer, decomp);
		else
			throw status;
	} catch (...) {
		decomp = -1;
	}

	delete[] buffer;
	delete[] destBuffer;

	return decomp;
}


