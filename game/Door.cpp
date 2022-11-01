#include "Door.h"

Door::Door(IE::door* areaDoor)
	:
	Object(areaDoor->name, Object::DOOR, areaDoor->script.CString()),
	fAreaDoor(areaDoor),
	fTiledObject(NULL)
{
}


res_ref
Door::ShortName() const
{
	return fAreaDoor->short_name;
}


void
Door::Toggle()
{
	if (Opened()) {
		fAreaDoor->flags &= ~IE::DOOR_OPEN;
		std::cout << "Door: (" << Name() << "): Close()" << std::endl;
	} else {
		fAreaDoor->flags |= IE::DOOR_OPEN;
		std::cout << "Door: (" << Name() << "): Open()" << std::endl;
	}
}


void
Door::Open(Object* actor)
{
	if (!(fAreaDoor->flags & IE::DOOR_OPEN)) {
		fAreaDoor->flags |= IE::DOOR_OPEN;
		AddTrigger(trigger_entry("Opened", actor));
	}
}


void
Door::Close(Object* actor)
{
	if (fAreaDoor->flags & IE::DOOR_OPEN) {
		fAreaDoor->flags &= ~IE::DOOR_OPEN;
		AddTrigger(trigger_entry("Closed", actor));
	}
}


void
Door::Lock()
{
	if (!(fAreaDoor->flags & IE::DOOR_LOCKED))
		fAreaDoor->flags |= IE::DOOR_LOCKED;
	std::cout << Name() << ": Lock()" << std::endl;
}


void
Door::Unlock()
{
	if (fAreaDoor->flags & IE::DOOR_LOCKED)
		fAreaDoor->flags &= ~IE::DOOR_LOCKED;
	std::cout << Name() << ": Unlock()" << std::endl;
}


/* virtual */
IE::point
Door::NearestPoint(const IE::point& start) const
{
	IE::point targetPoint;
	if (start.x <= fAreaDoor->player_box.x_min)
		targetPoint.x = fAreaDoor->player_box.x_min;
	else if (start.x >= fAreaDoor->player_box.x_max)
		targetPoint.x = fAreaDoor->player_box.x_max;
	if (start.y <= fAreaDoor->player_box.y_min)
		targetPoint.y = fAreaDoor->player_box.y_min;
	else if (start.y >= fAreaDoor->player_box.y_max)
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


::Outline
Door::Outline() const
{
	GFX::Color color = {10, 10, 50};
	return ::Outline(Frame(), color);
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
}
