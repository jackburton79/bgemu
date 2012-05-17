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


struct token {
	token();
	token(const char *tok);
	union {
		char string[32];
		int number;
	} u;
	int type;
	int size;
};


bool operator==(const struct token &t1, const struct token &t2);


enum block_type {
	BLOCK_SCRIPT,
	BLOCK_CONDITION_RESPONSE,
	BLOCK_CONDITION,
	BLOCK_TRIGGER,
	BLOCK_ACTION,
	BLOCK_OBJECT,
	BLOCK_REEESPONSE,
	BLOCK_RESPONSE,
	BLOCK_UNKNOWN
};


struct block {
	int type;
	uint32 offset;
	uint32 end;
};


enum element_type {
	ELEMENT_START,
	ELEMENT_TEXT,
	ELEMENT_END,
	ELEMENT_UNKNOWN
};


struct node;
typedef std::vector<node *> node_list;

struct node {
	node();
	void AddChild(node *child);
	int type;
	char header[3];
	char value[128];

	node_list children;
	bool closed;
};




class Stream;
class Tokenizer {
public:
	Tokenizer();
	Tokenizer(Stream *stream, int32 position);
	void SetTo(Stream *stream, int32 position);
	token ReadToken();
	token ReadNextToken();

	void RewindToken(token *tok);

	static bool IsSeparator(char const &c);
private:
	Stream *fStream;
	int32 fPosition;
};


class Parser {
public:
	Parser();
	void SetTo(Stream *stream);
	void SetTo(::block *block);
	token ReadToken();
	void PrintNode(node* n) const;
	void Read();
	block ReadBlock(const block *parentBlock = NULL);
	block ReadBlock(const int offset);

private:
	void _ReadTriggerBlock(int start, int end);
	void _ReadObjectBlock(int start, int end);
	bool _FindEndOfBlock(token blockHead, const uint32 &maxEnd, uint32 &position);
	void _SkipUselessTokens();

	void _ReadElementGuard(node* n);
	void _ReadElementValue(node* n);
	void _ReadElementChildren(node* n);
	static int _BlockTypeFromToken(const token &tok);

	node_list fNodes;
	//node_list::iterator fCurrentElement;
	Stream *fStream;
	Tokenizer *fTokenizer;
};


#endif // __PARSING_H
