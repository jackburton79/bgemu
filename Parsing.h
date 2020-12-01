#ifndef __PARSING_H
#define __PARSING_H

#include "IETypes.h"
#include "SupportDefs.h"

#include <vector>


enum token_type {
	TOKEN_END_OF_LINE,
	TOKEN_SPACE,
	TOKEN_STRING,
	TOKEN_NUMBER,
	TOKEN_QUOTED_STRING,
	TOKEN_EXCLAMATION_MARK,
	TOKEN_COMMA,
	TOKEN_PARENTHESIS,
	TOKEN_UNKNOWN
};


struct token {
	token();
	token(const char *tok);
	union {
		char string[48];
		int number;
	} u;
	int type;
	int size;
};


bool operator==(const struct token &t1, const struct token &t2);

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


struct object_node : public node {
	virtual void Print() const;
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

	object_node();
};


struct trigger_node : public node {
	virtual void Print() const;
	object_node* Object();
	int id;
	int parameter1;
	int flags;
	int parameter2;
	int unknown;
	char string1[48];
	char string2[48];

	trigger_node();
};


struct action_node : public node {
	virtual void Print() const;
	object_node* First();
	object_node* Second();
	object_node* Third();
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


class Stream;
class Tokenizer {
public:
	Tokenizer();
	Tokenizer(Stream *stream, int32 position);
	void SetTo(Stream *stream, int32 position);

	std::string TokenType(const token& t) const;

	token ReadToken();
	token ReadNextToken();
	void RewindToken(const token &tok);

	void SetDebug(bool state);

	static bool IsSeparator(char const &c);
private:
	void _SkipSeparators();
	int32 _ReadFullToken(char *dest, int32 start);

	Stream *fStream;
	int32 fPosition;
	bool fDebug;
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
	static bool TriggerFromString(const std::string& string, trigger_node& node);
	static bool ActionFromString(const std::string& string, action_node& node);

private:
	Parser(const Parser&);
	Parser& operator=(const Parser&);

	void _SkipUselessTokens();

	void _ReadNodeHeader(node*& n);
	void _ReadNode(node*& n);
	void _ReadNodeValue(node* n, const token& tok);

	static void _ReadTriggerBlock(Tokenizer *tokenizer, ::node* node);
	static void _ReadObjectBlock(Tokenizer *tokenizer, ::node* node);
	static void _ReadActionBlock(Tokenizer *tokenizer, ::node* node);
	static void _ReadResponseBlock(Tokenizer *tokenizer, ::node* node);

	static bool _ExtractTriggerName(Tokenizer& tokenizer, ::trigger_node* triggerNode);
	static token _ExtractNextParameter(Tokenizer& tokenizer, ::trigger_node* triggerNode,
									   Parameter& parameter);
	
	void _FixNode(::node *node);

	static int _BlockTypeFromToken(const token &tok);

	Stream *fStream;
	Tokenizer *fTokenizer;
	bool fDebug;
};

#endif // __PARSING_H
