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
	fDLG = gResManager->GetDLG(name);

}


Dialog::~Dialog()
{
	gResManager->ReleaseResource(fDLG);
}


void
Dialog::Start()
{
	fDLG->StartDialog();
}
