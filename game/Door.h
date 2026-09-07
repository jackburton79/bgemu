#ifndef __DOOR_H
#define __DOOR_H

#include "IETypes.h"
#include "Object.h"
#include "Polygon.h"


#include <vector>


class ARAResource;
class Door : public Object {
public:
	// area, when given, lets UpdateSearchMapBlocking() read the door's
	// real per-state "impeded cell block" data (see IESDP are_v1.htm) -
	// the exact cells the original map author marked blocked/passable
	// in each door state, indexed into the area's shared vertex table
	// (area->VertexAt()) by areaDoor's own open_cell_index/
	// closed_cell_index fields. Omit it (or pass NULL) to fall back to
	// the coarser OpenBox()/ClosedBox() bounding-box approximation -
	// used by callers that don't have an ARAResource at hand (there are
	// none in this codebase today; kept so Door stays constructible
	// without one).
	Door(IE::door* areaDoor, ARAResource* area = NULL);

	res_ref ShortName() const;	
	
	// TODO: Remove!
	void Toggle();

	void Open(Object* object);
	void Close(Object* object);

	// Marks this door's currently-active bounding box (OpenBox() when
	// open, ClosedBox() when closed) as impassable on its area's
	// SearchMap, and the other one passable. Called automatically by
	// Open()/Close() on every state change; also called once by
	// AreaRoom::_InitDoors() right after construction, to apply the
	// door's initial (map-authored) open/closed state.
	void UpdateSearchMapBlocking();

	void Lock();
	// `actor`, when given, is who unlocked it - posts an "Unlocked"
	// trigger naming them (see IESDP: "Unlocked(O:Object*) - returns true
	// only if this door was unlocked by the specified object"); omit it
	// for an untargeted trigger.
	void Unlock(Object* actor = NULL);

	bool IsLocked() const;
	uint32 LockDifficulty() const;
	res_ref KeyItem() const;

	bool IsTrapped() const;
	bool IsTrapDetected() const;
	void SetTrapDetected(bool detected);
	// `actor`, when given, is who disarmed it - posts a "Disarmed"
	// trigger naming them, same convention as Unlock() above.
	void DisarmTrap(Object* actor = NULL);
	uint16 TrapDetectionDifficulty() const;
	uint16 TrapRemovalDifficulty() const;

	bool IsSecret() const;
	bool IsDetected() const;
	void SetDetected(bool detected);
	uint32 DetectionDifficulty() const;

	std::vector<uint16> fTilesOpen;

	bool Opened() const;
	
	virtual IE::point NearestPoint(const IE::point& start) const;
	
	virtual IE::rect Frame() const;
	IE::rect OpenBox() const;
	IE::rect ClosedBox() const;
	
	virtual ::Outline Outline() const;

	const Polygon& OpenPolygon() const;
	const Polygon& ClosedPolygon() const;

	void Print() const;

private:
	IE::door* fAreaDoor;
	ARAResource* fAreaResource;
	IE::tiled_object* fTiledObject;
	Polygon fOpenPolygon;
	Polygon fClosedPolygon;
};

#endif // __DOOR_H
