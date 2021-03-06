/*
 * CHUIResource.cpp
 *
 *  Created on: 17/ott/2012
 *      Author: stefano
 */

#include "CHUIResource.h"

#include "Bitmap.h"
#include "Control.h"
#include "Log.h"
#include "MemoryStream.h"
#include "MOSResource.h"
#include "ResManager.h"
#include "Window.h"


#define CHU_SIGNATURE "CHUI"
#define CHU_VERSION_1 "V1  "


struct control_table {
	uint32 offset;
	uint32 length;
};


/* static */
Resource*
CHUIResource::Create(const res_ref& name)
{
	return new CHUIResource(name);
}


CHUIResource::CHUIResource(const res_ref &name)
	:
	Resource(name, RES_CHU),
	fNumWindows(-1),
	fControlTableOffset(0),
	fWindowsOffset(0)
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

	/*std::cout << Name() << ", " << fNumWindows << " windows" << std::endl;
	std::cout << "Windows offset: " << fWindowsOffset << std::endl;
	std::cout << "Controls table offset: " << fControlTableOffset << std::endl;
*/
	return true;
}


uint16
CHUIResource::CountWindows() const
{
	return fNumWindows;
}


Window*
CHUIResource::GetWindow(uint16 id)
{
	//std::cout << "CHUIResource::GetWindow(" << id << ")" << std::endl;
	Window* newWindow = NULL;
	try {
		// TODO: Not really efficient O(n): but shouldn't be critical
		IE::window window;
		for (uint32 n = 0; n < fNumWindows; n++) {
			fData->ReadAt(fWindowsOffset + n * sizeof(window), window);
			if (window.id == id)
				break;
		}
		if (window.id != id) {
			// window not found
			return NULL;
		}
		Bitmap* background = NULL;
		if (window.background) {
			MOSResource* mos = gResManager->GetMOS(window.background_mos);
			if (mos != NULL) {
				background = mos->Image();
				gResManager->ReleaseResource(mos);
			}
		}

		newWindow = new Window(window.id, window.x, window.y,
						window.w, window.h, background);

		//std::cout << "CHUIResource::GetWindow(): Window has ";
		//std::cout << std::dec << (int)window.num_controls << " controls." << std::endl;
		for (uint16 controlIndex = 0;
				controlIndex < window.num_controls; controlIndex++) {
			//std::cout << "Control " << controlIndex << ":" << std::endl;
			IE::control* control = _ReadControl(window, controlIndex);
			if (control != NULL)
				newWindow->Add(Control::CreateControl(control));
		}
	} catch (std::exception& e) {
		newWindow = NULL;
		std::cerr << Log::Red << "CHUIResource::GetWindow() FAILED: " << e.what() << std::endl;
	} catch (...) {
		newWindow = NULL;
		std::cerr << Log::Red << "CHUIResource::GetWindow() FAILED." << std::endl;
	}

	return newWindow;
}


void
CHUIResource::Dump()
{
	IE::window window;
	std::cout << Name() << ": " << fNumWindows << " windows" << std::endl;
	for (uint32 n = 0; n < fNumWindows; n++) {
		fData->ReadAt(fWindowsOffset + n * sizeof(window), window);
		window.Print();
		std::cout << "Controls:" << std::endl;
		for (uint16 controlIndex = 0;
				controlIndex < window.num_controls; controlIndex++) {
			//std::cout << "Control " << controlIndex << ":" << std::endl;
			IE::control* control = _ReadControl(window, controlIndex);
			if (control != NULL) {
				switch (control->type) {
					case IE::CONTROL_BUTTON:
						((IE::button*)control)->Print();
						break;
					case IE::CONTROL_LABEL:
						((IE::label*)control)->Print();
						break;
					case IE::CONTROL_TEXTAREA:
						((IE::text_area*)control)->Print();
						break;
					case IE::CONTROL_SLIDER:
						((IE::slider*)control)->Print();
						break;
					case IE::CONTROL_SCROLLBAR:
						((IE::scrollbar*)control)->Print();
						break;
					case IE::CONTROL_TEXTEDIT:
						((IE::text_edit*)control)->Print();
						break;
					default:
						control->Print();
						break;
				}
			}
		}
	}
}


IE::control*
CHUIResource::_ReadControl(IE::window& window, uint16 controlIndex)
{
	control_table controlTable;
	fData->ReadAt(fControlTableOffset
			+ (window.control_offset + controlIndex)
			* sizeof(controlTable), controlTable);

	IE::control* control = (IE::control*)new uint8[controlTable.length];
	fData->ReadAt(controlTable.offset, control, controlTable.length);

	return control;
}
