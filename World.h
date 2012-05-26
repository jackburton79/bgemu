#ifndef __WORLD_H
#define __WORLD_H

#include "IETypes.h"

#include <map>
#include <string>

struct action;
struct node;
struct trigger;

class Room;
class IDSResource;
class Script;
class TLKResource;
class World {
public:
	World();
	~World();

	void EnterArea(const char *name);
	Room *CurrentArea() const;

	void SetVariable(const char* name, int32 value);
	int32 GetVariable(const char* name);

	void AddScript(Script* script);
	void AddActorScript(const char* name, Script* script);
	void RemoveActorScript(const char* name);

	void CheckScripts();

private:
	Room *fCurrentRoom;

	std::map<std::string, uint32> fVariables;
	Script *fScript;
	std::map<std::string, Script*> fScripts;

	void _ExecuteScript(Script* script);
	bool _CheckTriggers(node* conditionNode);
	bool _EvaluateTrigger(trigger* trig);

	void _ExecuteActions(node* node);
	void _ExecuteAction(action* act);
};


World* GetWorld();

TLKResource* Dialogs();
IDSResource* GeneralIDS();
IDSResource* AnimateIDS();
IDSResource* GendersIDS();
IDSResource* RacesIDS();
IDSResource* ClassesIDS();
IDSResource* SpecificIDS();
IDSResource* TriggerIDS();
IDSResource* ActionIDS();


#endif // __WORLD_H
