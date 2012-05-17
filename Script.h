#ifndef __SCRIPT_H
#define __SCRIPT_H

#include <vector>

struct node;
typedef std::vector<node *> node_list;

struct node {
	node();
	~node();
	void AddChild(node *child);
	int type;
	char header[3];
	char value[128];

	node_list children;
	bool closed;
};

#endif
