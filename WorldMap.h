/*
 * WorldMap.h
 *
 *  Created on: 19/lug/2012
 *      Author: stefano
 */

#ifndef __WORLDMAP_H
#define __WORLDMAP_H

class WMAPResource;
class WorldMap {
public:
	WorldMap();
	~WorldMap();

	bool HandleUserInput();
	void Draw();
private:
	WMAPResource* fWmap;
};

#endif /* __WORLDMAP_H */
