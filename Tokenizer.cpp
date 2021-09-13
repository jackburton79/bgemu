/*
 * Tokenizer.cpp
 *
 *  Created on: 2 set 2021
 *      Author: Stefano Ceccherini
 */

#include "Tokenizer.h"

#include "Log.h"
#include "Stream.h"

#include <cstring>
#include <iostream>
#include <string>

// token
token::token()
	:
	type(TOKEN_UNKNOWN),
	size(0)
{
}


token::token(const char *tok)
	:
	type(TOKEN_UNKNOWN),
	size(0)
{
	::memcpy(u.string, tok, sizeof(u.string));
	size = ::strnlen(u.string, sizeof(u.string));

	if (!::memcmp(tok, "SC", 2)
		|| !::memcmp(tok, "CR", 2)
		|| !::memcmp(tok, "CO", 2)
		|| !::memcmp(tok, "TR", 2)
		|| !::memcmp(tok, "OB", 2)
		|| !::memcmp(tok, "RS", 2)
		|| !::memcmp(tok, "RE", 2)
		|| !::memcmp(tok, "AC", 2))
		type = TOKEN_STRING;
	else if (tok[0] == ' ')
		type = TOKEN_SPACE;
	else if (tok[0] == '\n' || tok[0] == '\r')
		type = TOKEN_END_OF_LINE;
	else if (tok[0] == '"')
		type = TOKEN_QUOTED_STRING;
	else if (tok[0] == '(')
		type = TOKEN_PARENTHESIS_OPEN;
	else if (tok[0] == ')')
		type = TOKEN_PARENTHESIS_CLOSED;
	else
		type = TOKEN_UNKNOWN;
}


bool
operator==(const struct token &t1, const struct token &t2)
{
	if (t1.type != t2.type)
		return false;

	switch (t1.type) {
		case TOKEN_QUOTED_STRING:
		case TOKEN_STRING:
		case TOKEN_PARENTHESIS_OPEN:
		case TOKEN_PARENTHESIS_CLOSED:
		case TOKEN_COMMA:
			return ::strncmp(t1.u.string, t2.u.string, sizeof(t1.u.string)) == 0;
		case TOKEN_NUMBER:
			return t1.u.number == t2.u.number;
		default:
			return false;
	}
}


// Tokenizer
Tokenizer::Tokenizer()
	:
	fStream(NULL),
	fPosition(0),
	fDebug(false)
{
}


Tokenizer::Tokenizer(Stream *stream, int32 position)
	:
	fDebug(false)
{
	SetTo(stream, position);
}


void
Tokenizer::SetTo(Stream *stream, int32 position)
{
	fStream = stream;
	fPosition = position;
	fStream->Seek(fPosition, SEEK_SET);
}


std::string
Tokenizer::TokenType(const token& t) const
{
	switch (t.type) {
		case TOKEN_END_OF_LINE:
			return "TOKEN_END_OF_LINE";
		case TOKEN_STRING:
			return "TOKEN_STRING";
		case TOKEN_SPACE:
			return "TOKEN_SPACE";
		case TOKEN_NUMBER:
			return "TOKEN_NUMBER";
		case TOKEN_QUOTED_STRING:
			return "TOKEN_QUOTED_STRING";
		case TOKEN_EXCLAMATION_MARK:
			return "TOKEN_EXCLAMATION_MARK";
		case TOKEN_COMMA:
			return "TOKEN_COMMA";
		case TOKEN_PARENTHESIS_OPEN:
			return "TOKEN_PARENTHESIS_OPEN";
		case TOKEN_PARENTHESIS_CLOSED:
			return "TOKEN_PARENTHESIS_CLOSED";
		default:
			return "TOKEN_UNKNOWN";
	}
}


token
Tokenizer::ReadToken()
{
	_SkipSeparators();

	char array[128];
	int32 startToken = fStream->Position();
	token aToken;
	aToken.size = _ReadFullToken(array, startToken);
	array[aToken.size] = '\0';

	if (array[0] == '!') {
		aToken.type = TOKEN_EXCLAMATION_MARK;
		memcpy(aToken.u.string, array, aToken.size);
		aToken.u.string[aToken.size] = '\0';
	} else if (array[0] == ',') {
		aToken.type = TOKEN_COMMA;
		memcpy(aToken.u.string, array, aToken.size);
		aToken.u.string[aToken.size] = '\0';
	} else if (array[0] == '(') {
		aToken.type = TOKEN_PARENTHESIS_OPEN;
		memcpy(aToken.u.string, array, aToken.size);
		aToken.u.string[aToken.size] = '\0';
	} else if (array[0] == ')') {
		aToken.type = TOKEN_PARENTHESIS_CLOSED;
		memcpy(aToken.u.string, array, aToken.size);
		aToken.u.string[aToken.size] = '\0';
	} else if (array[0] == '"') {
		aToken.type = TOKEN_QUOTED_STRING;
		memcpy(aToken.u.string, array, aToken.size);
		aToken.u.string[aToken.size] = '\0';
	} else if (isalpha(array[0])) {
		aToken.type = TOKEN_STRING;
		memcpy(aToken.u.string, array, aToken.size);
		aToken.u.string[aToken.size] = '\0';
	} else {
		aToken.type = TOKEN_NUMBER;
		char* rest = NULL;
		aToken.u.number = strtol(array, &rest, 0);
		if (rest != NULL)
			aToken.size = std::min((int)(rest - array), aToken.size);
	}

	if (fDebug) {
		std::cout << "token: type " << TokenType(aToken);
		std::cout << std::dec << ", size ";
		std::cout << aToken.size << ", value ";
		if (aToken.type == TOKEN_NUMBER)
			std::cout << aToken.u.number;
		else
			std::cout << "'* " << aToken.u.string << " *'";
		std::cout << std::endl;
	}
	// We could have read too much, rewind a bit if needed
	fStream->Seek(startToken + aToken.size, SEEK_SET);
	return aToken;
}


void
Tokenizer::RewindToken(const token &tok)
{
	if (tok.size != 0)
		fStream->Seek(-tok.size, SEEK_CUR);
}


void
Tokenizer::SetDebug(bool state)
{
	fDebug = state;
}


void
Tokenizer::_SkipSeparators()
{
	while (!fStream->Eof()) {
		char c = fStream->ReadByte();
		if (!Tokenizer::IsWhiteSpace(c))
			break;
	}

	fStream->Seek(-1, SEEK_CUR);
}


int32
Tokenizer::_ReadFullToken(char* dest, int32 start)
{
	bool quotesOpen = false;
	char* ptr = dest;
	try {
		while (!fStream->Eof()) {
			char c = fStream->ReadByte();
			if (Tokenizer::IsWhiteSpace(c)) {
				fStream->Seek(-1, SEEK_CUR);
				break;
			}
			*ptr++ = c;
			if (c == '"') {
				if (!quotesOpen) {
					quotesOpen = true;
				} else {
					break;
				}
			} else if (c == '!' || c == ',' || c == '(' || c == ')') {
				if (fStream->Position() > start + 1)
					fStream->Seek(-1, SEEK_CUR);
				break;
			}
		}
	} catch (std::exception& e) {
		std::cerr << Log::Red << e.what() << Log::Normal << std::endl;
		abort();
	}
	return fStream->Position() - start;
}


/* static */
bool
Tokenizer::IsWhiteSpace(char const &c)
{
	return (c == ' ' || c == '\n' || c == '\r' || c == '\t');
}


