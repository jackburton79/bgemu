#include "Door.h"

Door::Door(IE::door* areaDoor)
	:
	Object(areaDoor->name, areaDoor->script.CString()),
	fAreaDoor(areaDoor)
{
}


void
Door::Toggle()
{
	bool wasOpen = fAreaDoor->flags & IE::DOOR_OPEN;

	if (wasOpen)
		fAreaDoor->flags &= ~IE::DOOR_OPEN;
	else
		fAreaDoor->flags |= IE::DOOR_OPEN;
}


void
Door::Open(Object* actor)
{
	if (!(fAreaDoor->flags & IE::DOOR_OPEN)) {
		fAreaDoor->flags |= IE::DOOR_OPEN;
		//Core::Get()->CurrentRoundResults()->fOpenedBy = actor->Name();
	}
}


void
Door::Close(Object* actor)
{
	if (fAreaDoor->flags & IE::DOOR_OPEN) {
		fAreaDoor->flags &= ~IE::DOOR_OPEN;;
		//CurrentScriptRoundResults()->fClosedBy = actor->Name();
	}
}


/* virtual */
IE::point
Door::NearestPoint(const IE::point& start) const
{
	IE::point targetPoint;
	if (start.x < fAreaDoor->player_box.x_min)
		targetPoint.x = fAreaDoor->player_box.x_min;
	else if (start.x > fAreaDoor->player_box.x_max)
		targetPoint.x = fAreaDoor->player_box.x_max;
	if (start.y < fAreaDoor->player_box.y_min)
		targetPoint.y = fAreaDoor->player_box.y_min;
	else if (start.y > fAreaDoor->player_box.y_max)
		targetPoint.y = fAreaDoor->player_box.y_max;
			
	return targetPoint;
}


IE::rect
Door::Frame() const
{
	return Opened() ? OpenBox() : ClosedBox();
}


IE::rect
Door::OpenBox() const
{
    return fAreaDoor->open_box;
}  


IE::rect
Door::ClosedBox() const
{
    return fAreaDoor->closed_box;
}


const Polygon&
Door::OpenPolygon() const
{
	return fOpenPolygon;
}


const Polygon&
Door::ClosedPolygon() const
{
	return fClosedPolygon;
}


bool
Door::Opened() const
{
	return fAreaDoor->flags & IE::DOOR_OPEN;
}


void
Door::Print() const
{
	//printf("%s\n", (const char*)fAreaDoor->id);
}
