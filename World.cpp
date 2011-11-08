#include "World.h"

#include "IDSResource.h"
#include "ResManager.h"
#include "Room.h"
#include "TLKResource.h"

static TLKResource *sDialogs;
static IDSResource *sGeneral;
static IDSResource *sAnimate;
static IDSResource *sRaces;
static IDSResource *sGenders;
static IDSResource *sClasses;
static IDSResource *sSpecifics;


World::World()
	:
	fCurrentRoom(NULL)
{
	sDialogs = gResManager->GetTLK(kDialogResource);
	sAnimate = gResManager->GetIDS("ANIMATE");
	sGeneral = gResManager->GetIDS("GENERAL");
	sRaces = gResManager->GetIDS("RACE");
	sGenders = gResManager->GetIDS("GENDER");
	sClasses = gResManager->GetIDS("CLASS");
	sSpecifics = gResManager->GetIDS("SPECIFIC");
}


World::~World()
{
	gResManager->ReleaseResource(sDialogs);
	gResManager->ReleaseResource(sSpecifics);
	gResManager->ReleaseResource(sGenders);
	gResManager->ReleaseResource(sRaces);
	gResManager->ReleaseResource(sClasses);
	gResManager->ReleaseResource(sGeneral);
	gResManager->ReleaseResource(sAnimate);
	delete fCurrentRoom;
}


void
World::EnterArea(const char *name)
{
	fCurrentRoom = new Room();
	if (!fCurrentRoom->Load(name))
		throw false;
}


Room *
World::CurrentArea() const
{
	return fCurrentRoom;
}


// global functions
TLKResource *
Dialogs()
{
	return sDialogs;
}


IDSResource *
GeneralIDS()
{
	return sGeneral;
}


IDSResource *
AnimateIDS()
{
	return sAnimate;
}


IDSResource *
RacesIDS()
{
	return sRaces;
}


IDSResource *
GendersIDS()
{
	return sGenders;
}


IDSResource *
ClassesIDS()
{
	return sClasses;
}


IDSResource *
SpecificIDS()
{
	return sSpecifics;
}
