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

	void AddChild(node *child);
	node* Parent() const;
	node* Next() const;

	const char* Value() const;

	virtual void Print() const;

	int type;
	char header[3];
	char value[128];

	node* parent;
	node* next;
	node_list children;
	bool closed;
	
	void Acquire();
	void Release();
	
protected:
	node();
	node(const node&);
	node& operator=(const node&);
	virtual ~node();
	
private:
	int32 fRefCount;	
};

bool operator==(const node &, const node &);


struct object_params {
	object_params();
	object_params(const object_params& other);

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
};


struct trigger_params : public node {
	virtual void Print() const;
	object_params* Object() const;

	int id;
	int parameter1;
	int flags;
	int parameter2;
	int unknown;
	char string1[48];
	char string2[48];

	trigger_params();
	trigger_params(const trigger_params& other);
	virtual ~trigger_params();

	trigger_params& operator=(const trigger_params& other);

private:
	object_params* object;
};


struct action_params : public node {
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
	action_params();

private:
	object_params first;
	object_params second;
	object_params third;
};


struct response_node : public node {
	response_node();
	virtual void Print() const;
	int probability;
	std::vector<action_params*> actions;
};


struct response_set {
	std::vector<response_node*> resp;
};


struct condition_block {
	std::vector<trigger_params*> triggers;
};

struct condition_response {
	condition_block conditions;
	response_set responseSet;
};


class Parameter;
class Parser {
public:
	Parser();
	~Parser();
	void SetTo(Stream *stream);
	std::vector<condition_response*> Read();
	void SetDebug(bool debug);

	static std::vector<trigger_params*> TriggersFromString(const std::string& string);
	static trigger_params* TriggerFromString(const std::string& string);
	static bool ActionFromString(const std::string& string, action_params& node);

private:
	Parser(const Parser&);
	Parser& operator=(const Parser&);

	void _ReadNodeHeader(node*& n);
	void _ReadNode(node*& n);
	void _ReadNodeValue(node* n, const token& tok);

	condition_response* _ReadConditionResponseBlock();
	void _ReadConditionBlock(condition_block& param);
	trigger_params* _ReadTriggerBlock();

	void _ReadResponseSetBlock(response_set& respSet);
	response_node* _ReadResponseBlock();
	action_params* _ReadActionBlock();

	static void _ReadTriggerBlock(Tokenizer *tokenizer, ::node* node);
	static void _ReadActionBlock(Tokenizer *tokenizer, ::node* node);
	static void _ReadResponseBlock(Tokenizer *tokenizer, ::node* node);

	static void _ReadObjectBlock(Tokenizer *tokenizer, object_params& obj);

	static bool _ExtractTriggerName(Tokenizer& tokenizer, ::trigger_params* triggerNode);
	
	void _FixNode(::node *node);

	static int _BlockTypeFromToken(const token &tok);

	Stream *fStream;
	Tokenizer *fTokenizer;
	bool fDebug;
};

#endif // __PARSING_H
