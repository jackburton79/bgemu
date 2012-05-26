#ifndef __SCRIPT_H
#define __SCRIPT_H

#include <vector>

#include "IETypes.h"

struct node;
typedef std::vector<node *> node_list;

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
class Script {
public:
	Script(node *rootNode);
	~Script();

	void Print() const;

	node* RootNode();
	node* FindNode(block_type type, node* start);

	void SetProcessed();
	bool Processed() const;

	Actor* Target() const;
private:
	friend class Actor;

	void _PrintNode(node* n) const;

	node *fRootNode;
	node *fCurrentNode;

	Actor* fActor;
	bool fProcessed;

};


struct node {
	static node* Create(int type, const char *string);

	virtual ~node();
	void AddChild(node *child);
	node* Next() const;
	node* Previous() const;
	node* FindNode(block_type nodeType) const;

	virtual void Print() const;

	int type;
	char header[3];
	char value[128];

	node* parent;
	node_list children;
	bool closed;

protected:
	node();
};


struct trigger : public node {
	virtual void Print() const;
	int id;
	int parameter1;
	int flags;
	int parameter2;
	int unknown;
	char string1[32];
	char string2[32];

	trigger();
};


struct object : public node {
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
	int specifiers;
	char name[32];

	object();
};


struct action : public node {
	virtual void Print() const;
	int id;
	int parameter;
	point where;
	int e;
	int f;
	char string1[32];
	char string2[32];

	action();
};


struct response : public node {
	virtual void Print() const;
	int probability;

	response();
};


#endif
