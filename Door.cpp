#include "Door.h"

std::map<std::string, Door*> Door::sDoors;

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
		CurrentScriptRoundResults()->fOpenedBy = actor->Name();
	}
}


void
Door::Close(Object* actor)
{
	if (fAreaDoor->flags & IE::DOOR_OPEN) {
		fAreaDoor->flags &= ~IE::DOOR_OPEN;;
		CurrentScriptRoundResults()->fClosedBy = actor->Name();
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

	/*IE::point pointB = {
			fAreaDoor->player_box.x_max,
			fAreaDoor->player_box.y_max
	};*/

	return pointA;
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
	std::map<std::string, Door*>::const_iterator i;
	i = sDoors.find(name);
	if (i == sDoors.end())
		return NULL;
	return i->second;
}


/* static */
std::map<std::string, Door*>&
Door::List()
{
	return sDoors;
}
