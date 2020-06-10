#include "VVCResource.h"

#include "Bitmap.h"
#include "Graphics.h"
#include "GraphicsEngine.h"
#include "MemoryStream.h"
#include "Path.h"
#include "IETypes.h"
#include "Utils.h"
#include "ZLibDecompressor.h"

#include <assert.h>
#include <iostream>
#include <limits.h>

#define VVC_SIGNATURE "VVC "
#define VVC_VERSION_1 "V1.0"


/* static */
Resource*
VVCResource::Create(const res_ref& name)
{
	return new VVCResource(name);
}


VVCResource::VVCResource(const res_ref& name)
	:
	Resource(name, RES_VVC)
{
}


VVCResource::~VVCResource()
{
}


bool
VVCResource::Load(Archive *archive, uint32 key)
{
	if (!Resource::Load(archive, key))
		return false;

	if (CheckSignature(VVC_SIGNATURE)
		&& CheckVersion(VVC_VERSION_1))
			return true;

	return false;
}


res_ref
VVCResource::BAMName() const
{
	res_ref name;
	fData->ReadAt(8, &name, sizeof(name));
	return name; 
}


uint16
VVCResource::DisplayFlags()
{
	uint16 flags = 0;
	fData->ReadAt(24, &flags, sizeof(flags));
	return flags;
}


uint16
VVCResource::ColourFlags()
{
	uint16 flags = 0;
	fData->ReadAt(26, &flags, sizeof(flags));
	return flags;
}


uint32
VVCResource::CountFrames() const
{
	uint32 numFrames = 0;
	fData->ReadAt(92, &numFrames, sizeof(numFrames));
	return numFrames;
}


uint32
VVCResource::IntroSequenceIndex() const
{
	uint32 seq = 0;
	fData->ReadAt(104, &seq, sizeof(seq));
	return seq;
}


uint32
VVCResource::MiddleSequenceIndex() const
{
	uint32 seq = 0;
	fData->ReadAt(108, &seq, sizeof(seq));
	return seq;
}


uint32
VVCResource::EndingSequenceIndex() const
{
	uint32 seq = 0;
	fData->ReadAt(144, &seq, sizeof(seq));
	return seq;
}


uint32
VVCResource::XPosition()
{
	uint32 x = 0;
	fData->ReadAt(40, &x, sizeof(x));
	return x;
}


uint32
VVCResource::ZOffset()
{
	uint32 z = 0;
	fData->ReadAt(76, &z, sizeof(z));
	return z;
}


uint32
VVCResource::YPosition()
{
	uint32 y = 0;
	fData->ReadAt(44, &y, sizeof(y));
	return y;
}


void
VVCResource::Dump()
{
	std::cout << Name() << std::endl;
	std::cout << std::endl;
	std::cout << "bam name: " << BAMName() << std::endl;	
	std::cout << "num frames: " << CountFrames() << std::endl;	
	std::cout << "display flags: " << std::hex << DisplayFlags() << std::endl;
	std::cout << "colour flags: " << std::hex << ColourFlags() << std::endl;	
	res_ref paletteName;
	fData->ReadAt(68, &paletteName, sizeof(paletteName));
	std::cout << "Palette: ";
	std::cout << paletteName.CString() << std::endl;
	std::cout << std::dec;
	std::cout << "x: " << XPosition() << ", y: " << YPosition() << ", z: " << ZOffset();
	std::cout << std::endl;
}

