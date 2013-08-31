#ifndef __SCRIPT_H
#define __SCRIPT_H

#include <vector>

#include "IETypes.h"

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

class Object;
class Script {
public:
	Script(node *rootNode);
	~Script();

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

	bool Execute();
	void SetTarget(Object* object);

	void SetProcessed();
	bool Processed() const;

private:
	bool _CheckTriggers(node* conditionNode);
	bool _EvaluateTrigger(trigger_node* trig);

	void _ExecuteActions(node* node);
	void _ExecuteAction(action_node* act);
	void _PrintNode(node* n) const;
	void _DeleteNode(node* n);

	node *fRootNode;

	bool fProcessed;

	Object* fTarget;

	node* fCurrentNode;
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
	int parameter;
	IE::point where;
	int e;
	int f;
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
