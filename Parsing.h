#ifndef __PARSING_H
#define __PARSING_H

#include "SupportDefs.h"

#include <vector>


enum token_type {
	TOKEN_END_OF_LINE,
	TOKEN_SPACE,
	TOKEN_BLOCK_GUARD,
	TOKEN_NUMBER,
	TOKEN_STRING,
	TOKEN_UNKNOWN
};

struct node;
typedef std::vector<node*> node_list;

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


class Stream;
class Tokenizer {
public:
	Tokenizer();
	Tokenizer(Stream *stream, int32 position);
	void SetTo(Stream *stream, int32 position);

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


struct specific;
class Parser {
public:
	Parser();
	~Parser();
	void SetTo(Stream *stream);

	token ReadToken();
	void Read(node*& root);
	void PrintNode(node* n) const;

	void SetDebug(bool debug);

private:
	void _SkipUselessTokens();

	void _ReadElementGuard(node*& n);
	void _ReadElement(node*& n);
	void _ReadElementValue(node* n, const token& tok);

	static void _ReadTriggerBlock(Tokenizer *tokenizer, ::node* node);
	static void _ReadObjectBlock(Tokenizer *tokenizer, ::node* node);
	static void _ReadActionBlock(Tokenizer *tokenizer, ::node* node);
	static void _ReadResponseBlock(Tokenizer *tokenizer, ::node* node);

	void _FixNode(::node *node);

	static int _BlockTypeFromToken(const token &tok);

	Stream *fStream;
	Tokenizer *fTokenizer;
	bool fDebug;
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


struct action_node : public node {
	virtual void Print() const;
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


#endif // __PARSING_H
