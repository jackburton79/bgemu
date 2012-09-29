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

struct node;
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

	void _FixNodeTree(::node *node);

	static int _BlockTypeFromToken(const token &tok);

	Stream *fStream;
	Tokenizer *fTokenizer;
	bool fDebug;
};


#endif // __PARSING_H
