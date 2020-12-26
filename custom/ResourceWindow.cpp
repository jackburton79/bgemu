/*
 * ResourceWindow.cpp
 *
 *  Created on: 17/ago/2017
 *      Author: stefano
 */

#include "ListView.h"
#include "ResManager.h"
#include "ResourceWindow.h"

ResourceWindow::ResourceWindow()
	:
	Window(999, 0, 0, 300, 200, NULL)
{
	IE::control* control = new IE::control;
	control->x = 0;
	control->y = 0;
	control->w = 300;
	control->h = 200;
	ListView* listView = new ListView(control);
	StringList resourceList;
	gResManager->GetCachedResourcesList(resourceList);

	std::vector<std::string>::iterator i;
	for (i = resourceList.begin(); i != resourceList.end(); i++) {
		std::string string(*i);
		listView->AddItem(string);
	}
	Add(listView);
}


ResourceWindow::~ResourceWindow()
{
}

