#include "Door.h"


std::map<std::string, Door*> Door::sDoors;

Door::Door(IE::door* areaDoor)
	:
	Object(areaDoor->name),
	fAreaDoor(areaDoor)
{
}


void
Door::Toggle()
{
	//printf("door %s toggled\n", fDoor.name);

	bool wasOpen = fAreaDoor->flags & IE::DOOR_OPEN;

	if (wasOpen)
		fAreaDoor->flags &= ~IE::DOOR_OPEN;
	else
		fAreaDoor->flags |= IE::DOOR_OPEN;
}


bool
Door::Opened() const
{
	return fAreaDoor->flags & IE::DOOR_OPEN;
}


void
Door::Print() const
{
	printf("%s\n", (const char*)fAreaDoor->id);
}


/* static */
void
Door::Add(Door* door)
{
	sDoors[door->fAreaDoor->name] = door;
}


/* static */
Door*
Door::GetByName(const char* name)
{
	return sDoors[name];
}
