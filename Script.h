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

	static trigger_node*	FindTriggerNode(node* start);
	static action_node*		FindActionNode(node* start);
	static object_node*		FindObjectNode(node* start);
	//int32			FindMultipleObjectNodes(std::vector<object_node*>& list,
		//				node* start = NULL) const;
	static node*			FindNode(block_type type, node* start);

	// convenience call
	Object*					FindObject(node* start) const;

	bool Execute(bool &continuing);

	Object* Sender();
	void SetSender(Object* object);

	Object* ResolveIdentifier(const int id) const;
	Object* GetObject(Actor* source, object_node* node) const;

	Object* LastTrigger() const;

	void SetProcessed();
	bool Processed() const;

private:
	bool _EvaluateConditionNode(node* conditionNode);
	bool _EvaluateTrigger(trigger_node* trig);
	bool _HandleResponseSet(node* node);
	bool _HandleAction(action_node* act);
	
	void _PrintNode(node* n) const;
	void _DeleteNode(node* n);

	node *fRootNode;

	bool fProcessed;

	int fOrTriggers;
	
	Object* fSender;
	Object* fLastTrigger;

	node* fCurrentNode;

	static bool sDebug;
};


#endif
