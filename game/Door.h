#ifndef __DOOR_H
#define __DOOR_H

#include "IETypes.h"
#include "Object.h"
#include "Polygon.h"


#include <vector>


class Door : public Object {
public:
	Door(IE::door* areaDoor);

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
	void Unlock();

	bool IsLocked() const;
	uint32 LockDifficulty() const;
	res_ref KeyItem() const;

	bool IsTrapped() const;
	bool IsTrapDetected() const;
	void SetTrapDetected(bool detected);
	void DisarmTrap();
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
	IE::tiled_object* fTiledObject;	
	Polygon fOpenPolygon;
	Polygon fClosedPolygon;
};

#endif // __DOOR_H
