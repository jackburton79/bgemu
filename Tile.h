#ifndef __TILE_H
#define __TILE_H

class Door;
class TileMap;
class Tile {
public:
	Tile();
	void SetTileMap(TileMap *map);
	void SetDoor(Door *d);

	::Door *Door() const;
	::TileMap *TileMap() const;

private:
	Door *fDoor;
	TileMap *fTileMap;
};


#endif // __TILE_H
