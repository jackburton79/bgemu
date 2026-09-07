#include "Door.h"

#include "AreaResource.h"
#include "AreaRoom.h"
#include "SearchMap.h"

Door::Door(IE::door* areaDoor, ARAResource* area)
	:
	Object(areaDoor->name, Object::DOOR, areaDoor->script.CString()),
	fAreaDoor(areaDoor),
	fAreaResource(area),
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


// Prefers IESDP's per-door "impeded cell block" (open_cell_index/
// open_cell_count/closed_cell_index/closed_cell_count in IE::door) -
// the exact list of search-map cells the original area author marked
// blocked in each door state, indexed into the area's shared vertex
// table (fAreaResource->VertexAt(), already in cell units per IESDP:
// "these entries are x.y coordinates in the area search map"). Falls
// back to OpenBox()/ClosedBox() - the door's bounding rectangles - when
// there's no ARAResource to read the real data from, or a door's own
// impeded-cell lists are empty.
//
// The bounding-box fallback is a coarser approximation: OpenBox() and
// ClosedBox() commonly overlap by a few pixels (both are the door's
// *sprite* bounding box in each state, not its walkable-gap shape), and
// in that overlap the "currently blocked" box always wins - found via
// a real Check-Passable sweep across two different doors in AR0602
// that neither state actually opened a connected path through. The
// precise impeded-cell-block data doesn't have this problem: it's an
// exact cell list per state, not two overlapping rectangles.
void
Door::UpdateSearchMapBlocking()
{
	AreaRoom* room = Area();
	if (room == NULL)
		return;
	::SearchMap* searchMap = room->SearchMap();
	if (searchMap == NULL)
		return;

	if (fAreaResource != NULL) {
		uint32 blockedIndex = Opened() ? fAreaDoor->open_cell_index : fAreaDoor->closed_cell_index;
		uint16 blockedCount = Opened() ? fAreaDoor->open_cell_count : fAreaDoor->closed_cell_count;
		uint32 passableIndex = Opened() ? fAreaDoor->closed_cell_index : fAreaDoor->open_cell_index;
		uint16 passableCount = Opened() ? fAreaDoor->closed_cell_count : fAreaDoor->open_cell_count;

		if (blockedCount > 0 || passableCount > 0) {
			for (uint16 c = 0; c < passableCount; c++) {
				IE::point cell = fAreaResource->VertexAt(passableIndex + c);
				searchMap->SetCellPassable(cell.x, cell.y);
			}
			for (uint16 c = 0; c < blockedCount; c++) {
				IE::point cell = fAreaResource->VertexAt(blockedIndex + c);
				searchMap->SetCellBlocked(cell.x, cell.y);
			}
			return;
		}
	}

	const IE::rect blocked = Opened() ? OpenBox() : ClosedBox();
	const IE::rect passable = Opened() ? ClosedBox() : OpenBox();

	for (int32 y = passable.y_min; y <= passable.y_max; y += 12) {
		for (int32 x = passable.x_min; x <= passable.x_max; x += 16)
			searchMap->ForcePassable(x, y);
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
	IE::rect open = OpenBox();
	IE::rect closed = ClosedBox();
	std::cout << std::dec;
	std::cout << "\topen box: (" << open.x_min << "," << open.y_min << ")-("
		<< open.x_max << "," << open.y_max << ")" << std::endl;
	std::cout << "\tclosed box: (" << closed.x_min << "," << closed.y_min << ")-("
		<< closed.x_max << "," << closed.y_max << ")" << std::endl;
	if (fAreaResource != NULL) {
		std::cout << "\topen cells (" << fAreaDoor->open_cell_count << "):";
		for (uint16 c = 0; c < fAreaDoor->open_cell_count; c++) {
			IE::point cell = fAreaResource->VertexAt(fAreaDoor->open_cell_index + c);
			std::cout << " (" << cell.x << "," << cell.y << ")";
		}
		std::cout << std::endl;
		std::cout << "\tclosed cells (" << fAreaDoor->closed_cell_count << "):";
		for (uint16 c = 0; c < fAreaDoor->closed_cell_count; c++) {
			IE::point cell = fAreaResource->VertexAt(fAreaDoor->closed_cell_index + c);
			std::cout << " (" << cell.x << "," << cell.y << ")";
		}
		std::cout << std::endl;
	}
}
