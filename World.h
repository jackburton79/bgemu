#ifndef __WORLD_H
#define __WORLD_H

class Room;
class TLKResource;
class World {
public:
	World();
	~World();

	void EnterArea(const char *name);
	Room *CurrentArea() const;

private:
	Room *fCurrentRoom;
};


TLKResource *Dialogs();


#endif // __WORLD_H
