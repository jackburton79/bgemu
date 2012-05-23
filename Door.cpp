#include "Door.h"

Door::Door(door_wed *door)
	:
	fOpen(true)
{
	fDoor = *door;
}


void
Door::Toggle()
{
	//printf("door %s toggled\n", fDoor.name);

/*	bool wasOpen = fDoor->flags & DOOR_OPEN;

	if (wasOpen)
		fDoor->flags &= ~DOOR_OPEN;
	else
		fDoor->flags |= DOOR_OPEN;*/
	fOpen = !fOpen;
}


bool
Door::Opened() const
{
	return fOpen;
	//return fDoor->flags & DOOR_OPEN;
}
