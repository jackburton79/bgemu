#include "Core.h"
#include "IDSResource.h"
#include "Script.h"
#include "Parsing.h"

#include <algorithm>

static int sIndent = 0;
static void IndentMore()
{
	sIndent++;
}


static void IndentLess()
{
	if (--sIndent < 0)
		sIndent = 0;
}


static void PrintIndentation()
{
	for (int i = 0; i < sIndent; i++)
		printf(" ");
}


Script::Script(node* rootNode)
	:
	fRootNode(rootNode),
	fProcessed(false)
{
}


Script::~Script()
{
	delete fRootNode;
}


node*
Script::RootNode()
{
	return fRootNode;
}


void
Script::Print() const
{
	_PrintNode(fRootNode);
}


node*
Script::FindNode(block_type type, node* start) const
{
	return start->FindNode(type);
}


void
Script::SetProcessed()
{
	fProcessed = true;
}


bool
Script::Processed() const
{
	return fProcessed;
}


void
Script::_PrintNode(node* n) const
{
	PrintIndentation();
	printf("<%s>", n->header);
	n->Print();
	node_list::iterator c;
	IndentMore();
	for (c = n->children.begin(); c != n->children.end(); c++) {
		_PrintNode(*c);
	}
	IndentLess();
	PrintIndentation();
	printf("<%s/>\n", n->header);
}


// node
/* static */
node*
node::Create(int type, const char *string)
{
	node* newNode = NULL;
	switch (type) {
		case BLOCK_TRIGGER:
			newNode = new trigger_node;
			break;
		case BLOCK_OBJECT:
			newNode = new object_node;
			break;
		case BLOCK_ACTION:
			newNode = new action_node;
			break;
		case BLOCK_RESPONSE:
			newNode = new response_node;
			break;
		default:
			newNode = new node;
			break;
	}
	if (newNode != NULL) {
		newNode->type = type;
		strcpy(newNode->header, string);
	}
	return newNode;
}


// node
node::node()
{
	parent = NULL;
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
	child->parent = this;
	children.push_back(child);
}


node*
node::Next() const
{
	if (parent == NULL)
		return NULL;

	node_list::iterator i = std::find(parent->children.begin(),
			parent->children.end(), this);
	if (++i == parent->children.end())
		return NULL;
	return *i;
}


node*
node::FindNode(block_type nodeType) const
{
	if (type == nodeType)
		return const_cast<node*>(this);

	node_list::const_iterator i;
	for (i = children.begin(); i != children.end(); i++) {
		node *n = (*i)->FindNode(nodeType);
		if (n != NULL)
			return n;
	}

	return NULL;
}


void
node::Print() const
{
	//printf("header: %s\n", header);
	//printf("value: %s\n", value);
}


bool
operator==(const node &a, const node &b)
{
	return false;
}


// trigger
trigger_node::trigger_node()
{
}


void
trigger_node::Print() const
{
	printf("id:%d, flags: %d, parameter1: %d, parameter2: %d, %s %s\n",
		id, flags, parameter1, parameter2, string1, string2);
}


// object
object_node::object_node()
{

}


void
object_node::Print() const
{
	printf("Object:\n");
	if (Core::Get()->Game() == GAME_TORMENT) {
		printf("team: %d ", team);
		printf("faction: %d ", faction);
	}
	printf("ea: %s ", EAIDS()->ValueFor(ea));
	printf("general: %s ", GeneralIDS()->ValueFor(general));
	printf("race: %s ", RacesIDS()->ValueFor(race));
	printf("class: %s\n", ClassesIDS()->ValueFor(classs));
	printf("specific: %s ", SpecificIDS()->ValueFor(specific));
	printf("gender: %s ", GendersIDS()->ValueFor(gender));
	printf("alignment: %d ", alignment);
	//if (identifiers != 0)
	printf("identifiers: %s \n", ObjectsIDS()->ValueFor(identifiers));
	//if (Core::Get()->Game() != GAME_BALDURSGATE)
		//printf("point: %d %d ", point.x, point.y);
	printf("name: %s\n", name);
}


// action
action_node::action_node()
{
}


void
action_node::Print() const
{
	printf("id: %d, parameter: %d, point: (%d, %d), %d, %d, %s, %s\n",
			id, parameter, where.x, where.y, e, f, string1, string2);
}


//response
response_node::response_node()
{
}


void
response_node::Print() const
{
	printf("probability: %d\n", probability);
}
