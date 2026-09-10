/*
 * Container.h
 *
 *  Created on: 03/set/2013
 *      Author: stefano
 */

#ifndef CONTAINER_H_
#define CONTAINER_H_

#include "IETypes.h"
#include "Object.h"
#include "Polygon.h"

#include <vector>

class Container: public Object {
public:
	Container(IE::container* container);

	virtual IE::rect Frame() const;
	::Outline Outline() const;

	const ::Polygon& Polygon() const;

	// CONTAINERENABLE(O:Object,I:Bool*BOOLEAN) - bit 5 of the ARE
	// container flags ("Disabled" per are_v1.htm), same raw-flags
	// approach Door already uses for its own lock/trap state.
	bool IsEnabled() const;
	void SetEnabled(bool enabled);

	// The container's own items (its ARE item_first_index/item_count
	// slice of the area's shared item list - see ARAResource::
	// GetContainerAt(), which populates this at load time via
	// AddContainerItem()). Read-only for now: clicking a container logs
	// its contents but doesn't move them into the party's inventory yet
	// (no loot GUI exists - see the Fase 6 plan notes).
	uint32 ItemCount() const;
	const IE::item& ItemAt(uint32 index) const;
	void AddContainerItem(const IE::item& item);
	// Removes the item at `index`, handing it back in `out`. Returns
	// false for an out-of-range index. Used to loot the container into a
	// party member's inventory (USECONTAINER).
	bool TakeItemAt(uint32 index, IE::item& out);
	// Whole item list - read for the session cache (Game::AreaCache),
	// written back to restore it on area re-entry.
	const std::vector<IE::item>& ContainerItems() const;
	void SetContainerItems(std::vector<IE::item> items);

private:
	virtual ~Container();
	IE::container* fContainer;
	::Polygon fPolygon;
	std::vector<IE::item> fItems;
};

#endif /* CONTAINER_H_ */
