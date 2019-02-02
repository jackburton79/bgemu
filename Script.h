#ifndef __SCRIPT_H
#define __SCRIPT_H

#include <vector>

#include "IETypes.h"
#include "Reference.h"

struct node;
struct action_node;
struct object_node;
struct trigger_node;
typedef std::vector<node*> node_list;

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

	trigger_node*	FindTriggerNode(node* start = NULL) const;
	action_node*	FindActionNode(node* start = NULL) const;
	object_node*	FindObjectNode(node* start = NULL) const;
	int32			FindMultipleObjectNodes(std::vector<object_node*>& list,
						node* start = NULL) const;
	node*			FindNode(block_type type, node* start = NULL) const;

	// convenience call
	Object*			FindObject(node* start = NULL) const;

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


struct node {
	static node* Create(int type, const char *string);

	virtual ~node();
	void AddChild(node *child);
	node* Next() const;
	node* Previous() const;

	virtual void Print() const;

	int type;
	char header[3];
	char value[128];

	node* parent;
	node* next;
	node_list children;
	bool closed;

protected:
	node();
};

bool operator==(const node &, const node &);

struct trigger_node : public node {
	virtual void Print() const;
	int id;
	int parameter1;
	int flags;
	int parameter2;
	int unknown;
	char string1[48];
	char string2[48];

	trigger_node();
};


struct object_node : public node {
	virtual void Print() const;
	int team;
	int faction;
	int ea;
	int general;
	int race;
	int classs;
	int specific;
	int gender;
	int alignment;
	int identifiers[5];
	IE::point point;
	char name[48];

	object_node();
};


struct action_node : public node {
	virtual void Print() const;
	int id;
	int integer1;
	IE::point where;
	int integer2;
	int integer3;
	char string1[48];
	char string2[48];

	action_node();
};


struct response_node : public node {
	virtual void Print() const;
	int probability;

	response_node();
};


#endif
