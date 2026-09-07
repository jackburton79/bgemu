#include "Door.h"

#include "AreaRoom.h"
#include "SearchMap.h"

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
		UpdateSearchMapBlocking();
	}
}


void
Door::Close(Object* actor)
{
	if (fAreaDoor->flags & IE::DOOR_OPEN) {
		fAreaDoor->flags &= ~IE::DOOR_OPEN;
		AddTrigger(trigger_entry("Closed", actor));
		UpdateSearchMapBlocking();
	}
}


// IESDP documents a per-door "impeded cell block" (open_cell_index/
// closed_cell_index + counts in IE::door) giving the door's exact
// search-map-blocking polygon, indexed into the area's shared vertex
// table - this codebase doesn't parse that table for doors (see
// resources/AreaResource.cpp's _LoadDoors(), which already reads the
// open/closed *outline* polygon the same way but never stores the
// result either). Using OpenBox()/ClosedBox() instead - the door's
// already-parsed, already-exposed bounding rectangles - is a coarser
// but much simpler approximation: it blocks/frees a rectangle instead of
// the door's exact shape.
void
Door::UpdateSearchMapBlocking()
{
	AreaRoom* room = Area();
	if (room == NULL)
		return;
	::SearchMap* searchMap = room->SearchMap();
	if (searchMap == NULL)
		return;

	const IE::rect blocked = Opened() ? OpenBox() : ClosedBox();
	const IE::rect passable = Opened() ? ClosedBox() : OpenBox();

	for (int32 y = passable.y_min; y <= passable.y_max; y += 12) {
		for (int32 x = passable.x_min; x <= passable.x_max; x += 16)
			searchMap->ClearPoint(x, y);
	}
	for (int32 y = blocked.y_min; y <= blocked.y_max; y += 12) {
		for (int32 x = blocked.x_min; x <= blocked.x_max; x += 16)
			searchMap->SetPoint(x, y);
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
Door::Unlock(Object* actor)
{
	if (fAreaDoor->flags & IE::DOOR_LOCKED) {
		fAreaDoor->flags &= ~IE::DOOR_LOCKED;
		AddTrigger(actor != NULL ? trigger_entry("Unlocked", actor) : trigger_entry("Unlocked"));
	}
	std::cout << Name() << ": Unlock()" << std::endl;
}


bool
Door::IsLocked() const
{
	return fAreaDoor->flags & IE::DOOR_LOCKED;
}


uint32
Door::LockDifficulty() const
{
	return fAreaDoor->lock_difficulty;
}


res_ref
Door::KeyItem() const
{
	return fAreaDoor->key_item;
}


bool
Door::IsTrapped() const
{
	return fAreaDoor->trapped != 0;
}


bool
Door::IsTrapDetected() const
{
	return fAreaDoor->trap_detected != 0;
}


void
Door::SetTrapDetected(bool detected)
{
	fAreaDoor->trap_detected = detected ? 1 : 0;
}


void
Door::DisarmTrap(Object* actor)
{
	fAreaDoor->trapped = 0;
	AddTrigger(actor != NULL ? trigger_entry("Disarmed", actor) : trigger_entry("Disarmed"));
}


uint16
Door::TrapDetectionDifficulty() const
{
	return fAreaDoor->trap_detection;
}


uint16
Door::TrapRemovalDifficulty() const
{
	return fAreaDoor->trap_removal;
}


bool
Door::IsSecret() const
{
	return fAreaDoor->flags & IE::DOOR_SECRET;
}


bool
Door::IsDetected() const
{
	return fAreaDoor->flags & IE::DOOR_DETECTED;
}


void
Door::SetDetected(bool detected)
{
	if (detected)
		fAreaDoor->flags |= IE::DOOR_DETECTED;
	else
		fAreaDoor->flags &= ~IE::DOOR_DETECTED;
}


uint32
Door::DetectionDifficulty() const
{
	return fAreaDoor->detection_difficulty;
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
	std::cout << "Door " << Name() << ": " << std::endl;
	std::cout << "\topen: " << (Opened() ? "yes" : "no") << std::endl;
	std::cout << "\tlocked: " << (IsLocked() ? "yes" : "no");
	std::cout << " (difficulty: " << LockDifficulty() << ")" << std::endl;
	std::cout << "\ttrapped: " << (IsTrapped() ? "yes" : "no") << std::endl;
}
