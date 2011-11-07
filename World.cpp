#include "World.h"

#include "ResManager.h"
#include "Room.h"
#include "TLKResource.h"

static TLKResource *sDialogs;


World::World()
	:
	fCurrentRoom(NULL)
{
	sDialogs = gResManager->GetTLK();
}


World::~World()
{
	gResManager->ReleaseResource(sDialogs);
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


TLKResource *
Dialogs()
{
	return sDialogs;
}
