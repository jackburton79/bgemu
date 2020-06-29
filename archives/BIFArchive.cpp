#include "BIFArchive.h"

#include "FileStream.h"
#include "MemoryStream.h"
#include "ZLibDecompressor.h"

#include <assert.h>
#include <cstring>
#include <iostream>


#define BIF_SIGNATURE "BIFFV1  "
#define BIFC_SIGNATURE "BIFCV1.0"


BIFArchive::BIFArchive(const char *fileName)
	:
	fStream(NULL)
{
	fStream = new FileStream(fileName, FileStream::READ_ONLY | FileStream::IGNORE_CASE);
	
	char signature[9];
	signature[8] = '\0';
	fStream->Read(signature, 8);
		
	if (!strcmp(signature, BIFC_SIGNATURE)) {
		Stream *tmpStream = fStream;

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
		std::cerr << "BIFArchive::BIFArchive: Unknown archive signature ";
		std::cerr << signature << std::endl;
		throw -1;
	}

	// if we're here: either the file was a compressed BIFF file and we
	// uncompressed it, so we have a normal BIF file, or it was already
	// uncompressed.
	
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
		std::cout << "Resource " << std::dec << c << " : size = " << size;
		std::cout << ", offset = " << offset << ", type: ";
		std::cout << strresource(type) << "(0x" << std::hex << type << ")";
		
		fStream->ReadAt(offset, name, 8);
		
		std::cout << " name : " << name << std::endl << dec; 
	}
	
	fStream->Seek(fTileEntriesOffset, SEEK_SET);
	for (uint32 c = 0; c < fNumTilesetEntries; c++) {
		(*fStream) >> locator >> offset >> numTiles >> tileSize >> type >> unk;
		std::cout << "Resource " << std::dec << c << " : numTiles = " << numTiles;
		std::cout << ", offset = " << offset << ", type: ";
		std::cout << strresource(type);
		
		fStream->ReadAt(offset, name, 8);
		
		std::cout << " name : " << name << std::endl << std::dec; 
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
	if (!is_tileset(type)) {
		index = RES_BIF_FILE_INDEX(key);
		resource_info info;
		if (!_GetResourceInfo(info, index))
			return NULL;
		size = info.size;
		offset = info.offset;
	} else {
		index = RES_TILESET_INDEX(key);
		tileset_info info;
		if (!_GetTilesetInfo(info, index))
			return NULL;
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
	size_t decompressedSize = decomp;
	try {
		buffer = new uint8[comp];
		destBuffer = new uint8[decomp];

		ssize_t read = source.Read(buffer, comp);
		if (read < 0)
			throw read;
		else if ((uint32)read != comp)
			throw -1;

		status_t status = ZLibDecompressor::DecompressBuffer(
				buffer, comp, destBuffer, decompressedSize);

		if (status == 0) {
			ssize_t write = dest.Write(destBuffer, decompressedSize);
			if (write < 0)
				throw write;
			else if ((size_t)write != decompressedSize)
				throw -1;
		} else
			throw status;
	} catch (...) {
		decompressedSize = (size_t)-1;
	}

	delete[] buffer;
	delete[] destBuffer;

	return (ssize_t)decompressedSize;
}


