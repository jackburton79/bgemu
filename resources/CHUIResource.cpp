/*
 * CHUIResource.cpp
 *
 *  Created on: 17/ott/2012
 *      Author: stefano
 */

#include "CHUIResource.h"
#include "MemoryStream.h"
#include "MOSResource.h"
#include "ResManager.h"

#define CHU_SIGNATURE "CHUI"
#define CHU_VERSION_1 "V1  "


struct control_table {
	uint32 offset;
	uint32 length;
};


CHUIResource::CHUIResource(const res_ref &name)
	:
	Resource(name, RES_CHU)
{
}


CHUIResource::~CHUIResource()
{
}


/* virtual */
bool
CHUIResource::Load(Archive *archive, uint32 key)
{
	if (!Resource::Load(archive, key))
		return false;

	if (!CheckSignature(CHU_SIGNATURE))
		return false;

	if (!CheckVersion(CHU_VERSION_1))
		return false;

	fData->ReadAt(8, fNumWindows);
	fData->ReadAt(12, fControlTableOffset);
	fData->ReadAt(16, fWindowsOffset);

	return true;
}


uint32
CHUIResource::CountWindows() const
{
	return fNumWindows;
}


Window*
CHUIResource::GetWindow(uint32 num)
{
	if (num >= fNumWindows)
		return NULL;

	IE::window window;
	fData->ReadAt(fWindowsOffset + num * sizeof(window), window);
	Window* newWindow = new Window();

	if (window.background) {
		MOSResource* mos = gResManager->GetMOS(window.background_mos);
		newWindow->fBackground = mos->Image();
		gResManager->ReleaseResource(mos);
	}
	return newWindow;
}


void
CHUIResource::Dump()
{
	IE::window window;
	for (uint32 n = 0; n < fNumWindows; n++) {
		fData->ReadAt(fWindowsOffset + n * sizeof(window), window);
		printf("window %d:\n", n);
		window.Print();
	}
}


// Window
Window::Window()
{

}


Bitmap*
Window::Background()
{
	return fBackground;
}
