#include "Script.h"
#include "Parsing.h"


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
	fCurrentNode(NULL)
{

}


void
Script::Print() const
{
	_PrintNode(fRootNode);
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
			newNode = new trigger;
			break;
		case BLOCK_OBJECT:
			newNode = new object;
			break;
		case BLOCK_ACTION:
			newNode = new action;
			break;
		case BLOCK_RESPONSE:
			newNode = new response;
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


void
node::Print() const
{
	printf("%s\n", value);
}


// trigger
trigger::trigger()
{
}


void
trigger::Print() const
{
	printf("id:%d, flags: %d, parameter1: %d, parameter2: %d, %s %s\n",
		id, flags, parameter1, parameter2, string1, string2);
}


// object
object::object()
{

}


void
object::Print() const
{
	printf("team: %d, faction: %d, ea: %d, general: %d, race: %d\n",
			team, faction, ea, general, race);
	printf("class: %d, specific: %d, gender: %d, alignment: %d, specifiers: %d\n",
			classs, specific, gender, alignment, specifiers);
	printf("name: %s\n", name);
}


// action
action::action()
{
}


void
action::Print() const
{
	printf("id: %d, parameter: %d, point: (%d, %d), %d, %d, %s, %s\n",
			id, parameter, where.x, where.y, e, f, str1, str2);
}


//response
response::response()
{
}


void
response::Print() const
{
	printf("probability: %d\n", probability);
}
