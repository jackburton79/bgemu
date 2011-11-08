#ifndef __WORLD_H
#define __WORLD_H

class Room;
class IDSResource;
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
IDSResource *GeneralIDS();
IDSResource *AnimateIDS();
IDSResource *GendersIDS();
IDSResource *RacesIDS();
IDSResource *ClassesIDS();
IDSResource *SpecificIDS();


#endif // __WORLD_H
