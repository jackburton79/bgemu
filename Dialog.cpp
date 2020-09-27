/*
 * Dialog.cpp
 *
 *  Created on: 22 set 2020
 *      Author: Stefano Ceccherini
 */

#include "Dialog.h"

#include "DLGResource.h"
#include "ResManager.h"


Dialog::Dialog(const res_ref& name)
{
	std::cout << "Dialog(" << name << ")" << std::endl;
	fDLG = gResManager->GetDLG(name);
	std::cout << "OK" << std::endl;
}


Dialog::~Dialog()
{
	gResManager->ReleaseResource(fDLG);
}


void
Dialog::Start()
{
	if (fDLG == NULL)
		std::cout << "Dialog::Start(): BUG! fDLG is empty!!!!" << std::endl;
	else
		fDLG->StartDialog();
}
