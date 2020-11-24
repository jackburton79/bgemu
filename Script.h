#ifndef __SCRIPT_H
#define __SCRIPT_H

#include <vector>

#include "IETypes.h"
#include "Parsing.h"
#include "Reference.h"


enum block_type {
	BLOCK_SCRIPT,
	BLOCK_CONDITION_RESPONSE,
	BLOCK_CONDITION,
	BLOCK_TRIGGER,
	BLOCK_ACTION,
	BLOCK_OBJECT,
	BLOCK_RESPONSESET,
	BLOCK_RESPONSE,
	BLOCK_UNKNOWN
};

class Actor;
class Object;
class Script {
public:
	Script(node *rootNode);
	~Script();

	static void SetDebug(bool debug);

	void Print() const;

	void Add(Script* script);

	static trigger_node*	FindTriggerNode(Object* object, node* start);
	static action_node*		FindActionNode(Object* object, node* start);
	static object_node*		FindObjectNode(const Object* object, node* start);
	static node*			FindNode(const Object* object, block_type type, node* start);

	// convenience calls
	static Object*			FindTriggerObject(const Object* object, trigger_node* start);
	static Object*			FindSenderObject(const Object* object, action_node* start);
	static Object*			FindTargetObject(const Object* object, action_node* start);
	
	bool Execute(bool &continuing);

	Object* Sender();
	void SetSender(Object* object);

	static bool EvaluateTrigger(Object* sender, trigger_node* trig, int& orTrig);

	static Object* ResolveIdentifier(const Object* object, object_node* node, const int id);
	static Object* GetObject(const Object* source, object_node* node);
	
	bool IsActionInstant(uint16 actionId) const;
private:
	bool _EvaluateConditionNode(node* conditionNode);

	bool _HandleResponseSet(node* node);
	bool _HandleAction(action_node* act);
	
	void _PrintNode(node* n) const;
	void _DeleteNode(node* n);

	node *fRootNode;
	
	Object* fSender;

	node* fCurrentNode;

	static bool sDebug;
};


#endif
