#include "Door.h"

Door::Door(IE::door* areaDoor)
	:
	Object(areaDoor->name,areaDoor->script.CString()),
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


IE::point
Door::NearestPoint(const IE::point& point) const
{
	IE::point pointA = {
			fAreaDoor->player_box.x_min,
			fAreaDoor->player_box.y_min
	};

	IE::point pointB = {
			fAreaDoor->player_box.x_max,
			fAreaDoor->player_box.y_max
	};

	if (point - pointA < point - pointB)
		return pointA;

	return pointB;
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
