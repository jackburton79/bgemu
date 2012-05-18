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



class Stream;
class Tokenizer {
public:
	Tokenizer();
	Tokenizer(Stream *stream, int32 position);
	void SetTo(Stream *stream, int32 position);
	token ReadToken();
	token ReadNextToken();

	void RewindToken(const token &tok);

	static bool IsSeparator(char const &c);

private:
	void _SkipSeparators();
	int32 _ReadFullToken(char *dest, int32 start);

	Stream *fStream;
	int32 fPosition;


};

struct node;
struct specific;
class Parser {
public:
	Parser();
	void SetTo(Stream *stream);

	token ReadToken();
	void Read(node*& root);
	void PrintNode(node* n) const;

	static node* CreateNode(int type, const char* string);

private:
	void _SkipUselessTokens();

	void _ReadElementGuard(node*& n);
	void _ReadElement(node*& n);
	void _ReadElementValue(node* n, const token& tok);

	void _ReadTriggerBlock(::node *node);
	void _ReadObjectBlock(int start, int end);

	static int _BlockTypeFromToken(const token &tok);


	Stream *fStream;
	Tokenizer *fTokenizer;
};


#endif // __PARSING_H
