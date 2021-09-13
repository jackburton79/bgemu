#ifndef __PARSING_H
#define __PARSING_H

#include "IETypes.h"
#include "SupportDefs.h"
#include "Tokenizer.h"

#include <vector>


struct node;
typedef std::vector<node*> node_list;

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
	node(const node&);
	node& operator=(const node&);
};

bool operator==(const node &, const node &);


struct object_params {
	void Print() const;
	bool Empty() const;
	
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

	object_params();
};


struct trigger_node : public node {
	virtual void Print() const;
	object_params* Object();
	int id;
	int parameter1;
	int flags;
	int parameter2;
	int unknown;
	char string1[48];
	char string2[48];
	object_params object;

	trigger_node();
};


struct action_node : public node {
	virtual void Print() const;
	object_params* First();
	object_params* Second();
	object_params* Third();
	int id;
	int integer1;
	IE::point where;
	int integer2;
	int integer3;
	char string1[48];
	char string2[48];
	object_params first;
	object_params second;
	object_params third;

	action_node();
};


struct response_node : public node {
	virtual void Print() const;
	int probability;

	response_node();
};


class Parameter;
class Parser {
public:
	Parser();
	~Parser();
	void SetTo(Stream *stream);

	token ReadToken();
	void Read(node*& root);
	void PrintNode(node* n) const;
	
	void SetDebug(bool debug);

	static std::vector<trigger_node*> TriggersFromString(const std::string& string);
	static trigger_node* TriggerFromString(const std::string& string);
	static bool ActionFromString(const std::string& string, action_node& node);

private:
	Parser(const Parser&);
	Parser& operator=(const Parser&);

	void _ReadNodeHeader(node*& n);
	void _ReadNode(node*& n);
	void _ReadNodeValue(node* n, const token& tok);

	static void _ReadTriggerBlock(Tokenizer *tokenizer, ::node* node);
	static void _ReadActionBlock(Tokenizer *tokenizer, ::node* node);
	static void _ReadResponseBlock(Tokenizer *tokenizer, ::node* node);

	static void _ReadObjectBlock(Tokenizer *tokenizer, object_params& obj);

	static bool _ExtractTriggerName(Tokenizer& tokenizer, ::trigger_node* triggerNode);
	
	void _FixNode(::node *node);

	static int _BlockTypeFromToken(const token &tok);

	Stream *fStream;
	Tokenizer *fTokenizer;
	bool fDebug;
};

#endif // __PARSING_H
