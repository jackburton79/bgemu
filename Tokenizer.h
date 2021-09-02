/*
 * Tokenizer.h
 *
 *  Created on: 2 set 2021
 *      Author: Stefano Ceccherini
 */

#ifndef TOKENIZER_H_
#define TOKENIZER_H_

#include "SupportDefs.h"

#include <string>

enum token_type {
	TOKEN_END_OF_LINE,
	TOKEN_SPACE,
	TOKEN_STRING,
	TOKEN_NUMBER,
	TOKEN_QUOTED_STRING,
	TOKEN_EXCLAMATION_MARK,
	TOKEN_COMMA,
	TOKEN_PARENTHESIS_OPEN,
	TOKEN_PARENTHESIS_CLOSED,
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


#endif /* TOKENIZER_H_ */
