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

class Action;
class Actor;
class Object;
class Script {
public:
	Script(std::vector<condition_response*> root);
	~Script();

	static void SetDebug(bool debug);

	void Print() const;

	//static trigger_params*	FindTriggerNode(Object* object, node* start);
	//static action_params*		FindActionNode(Object* object, node* start);
	//static node*			FindNode(const Object* object, block_type type, node* start);

	// convenience calls
	static Object*			GetTriggerObject(const Object* object, trigger_params* start);
	static Object*			GetSenderObject(const Object* object, action_params* start);
	static Object*			GetTargetObject(const Object* object, action_params* start);
	
	void Execute(bool &continuing, bool& action);

	Object* Sender();
	void SetSender(Object* object);

	static bool EvaluateTrigger(Object* sender, trigger_params* trig, int& orTrig);

	static Object* ResolveIdentifier(const Object* object, object_params* node, const int id);
	static Object* GetObject(const Object* source, object_params* node);
	
private:
	bool _EvaluateConditionNode(condition_block& block);

	bool _HandleResponseSet(response_set& responseSet);
	bool _HandleAction(action_params* act);
	Action* _GetAction(Object* sender, action_params* act, bool& isContinue);
	
	//void _PrintNode(node* n) const;
	//void _DeleteNode(node* n);
	static Actor* _GetIdentifiers(const Object* source, object_params* node,
					std::vector<std::string>& identifiersList);
	
	std::vector<condition_response*> fConditionResponses;
	Object* fSender;

	static bool sDebug;
};


#endif
