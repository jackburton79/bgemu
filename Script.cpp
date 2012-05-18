#include "Script.h"
#include "Parsing.h"

// node
node::node()
{
	closed = false;
	type = BLOCK_UNKNOWN;
	value[0] = '\0';
}


node::~node()
{
	node_list::iterator i;
	for (i = children.begin(); i != children.end(); i++)
		delete (*i);
}


void
node::AddChild(node* child)
{
	children.push_back(child);
}


trigger::trigger()
{

}
