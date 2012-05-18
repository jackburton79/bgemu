#ifndef __SCRIPT_H
#define __SCRIPT_H

#include <vector>

struct node;
typedef std::vector<node *> node_list;

struct node {
	node();
	virtual ~node();
	void AddChild(node *child);
	int type;
	char header[3];
	char value[128];

	node_list children;
	bool closed;
};


struct trigger : public node {
	trigger();
	int id;
	int parameter1;
	int flags;
	int parameter2;
	int unknown;
	char string1[32];
	char string2[32];
};

struct object : public node {

};

#endif
