#include "Parsing.h"
#include "Script.h"
#include "Stream.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <string>



// token
token::token()
	:
	type(TOKEN_UNKNOWN),
	size(0)
{
}


token::token(const char *tok)
{
	strncpy(u.string, tok, sizeof(u.string));

	if (!memcmp(tok, "SC", 2)
		|| !memcmp(tok, "CR", 2)
		|| !memcmp(tok, "CO", 2)
		|| !memcmp(tok, "TR", 2)
		|| !memcmp(tok, "OB", 2)
		|| !memcmp(tok, "RS", 2)
		|| !memcmp(tok, "RE", 2)
		|| !memcmp(tok, "AC", 2))
		type = TOKEN_BLOCK_GUARD;
	else if (tok[0] == ' ')
		type = TOKEN_SPACE;
	else if (tok[0] == '\n' || tok[0] == '\r')
		type = TOKEN_END_OF_LINE;
	else if (tok[0] == '"')
		type = TOKEN_STRING;
	else
		type = TOKEN_UNKNOWN;
}


bool
operator==(const struct token &t1, const struct token &t2)
{
	if (t1.type != t2.type)
		return false;

	switch (t1.type) {
		case TOKEN_BLOCK_GUARD:
			return memcmp((const void*)t1.u.string, (const void*)t2.u.string, 2) == 0;
		case TOKEN_STRING:
			return strncmp(t1.u.string, t2.u.string, sizeof(t1.u.string)) == 0;
		case TOKEN_NUMBER:
			return t1.u.number == t2.u.number;
		default:
			return false;
	}
}

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

// Parser
Parser::Parser()
	:
	fStream(NULL),
	fTokenizer(NULL)
{
	fTokenizer = new Tokenizer();
}


void
Parser::SetTo(Stream *stream)
{
	fStream = stream;
	fStream->Seek(0, SEEK_SET);

	fTokenizer->SetTo(stream, 0);
}


void
Parser::SetTo(::block *block)
{
	fStream->Seek(block->offset, SEEK_SET);
	fTokenizer->SetTo(fStream, block->offset);
}


void
Parser::PrintNode(node* n) const
{
	PrintIndentation();
	printf("<%s> %s\n", n->header, n->value);
	node_list::iterator c;
	IndentMore();
	for (c = n->children.begin(); c != n->children.end(); c++) {
		PrintNode(*c);
	}
	IndentLess();
	PrintIndentation();
	printf("<%s/>\n", n->header);
}


void
Parser::Read(node *rootNode)
{
	try {
		_ReadElementValue(rootNode);
	} catch (const char *str) {
		printf("caught string %s\n", str);
	} catch (...) {
		printf("end of file!\n");
	}

	PrintNode(rootNode);
}


void
Parser::_ReadTriggerBlock(int start, int end)
{
	Tokenizer t(fStream, start);
	token tok = t.ReadToken();
	assert(tok.type == TOKEN_NUMBER);

	printf("trigger id: %d\n", tok.u.number);

	tok = t.ReadNextToken();
	assert(tok.type == TOKEN_NUMBER);

	printf("parameter 1: %d\n", tok.u.number);
	tok = t.ReadNextToken();
	assert(tok.type == TOKEN_NUMBER);
	printf("flags: %d\n", tok.u.number);

	tok = t.ReadNextToken();
	assert(tok.type == TOKEN_NUMBER);
	printf("parameter 2: %d\n", tok.u.number);

	tok = t.ReadNextToken();
	assert(tok.type == TOKEN_NUMBER);
	printf("unknown: %d\n", tok.u.number);

	tok = t.ReadNextToken();
	assert(tok.type == TOKEN_STRING);
	printf("string: %s\n", tok.u.string);

	tok = t.ReadNextToken();
	assert(tok.type == TOKEN_STRING);
	printf("string: %s\n", tok.u.string);

	_ReadObjectBlock(fStream->Position() + 1, fStream->Position() + 10);
}


void
Parser::_ReadObjectBlock(int start, int end)
{
	Tokenizer t(fStream, start);
	token tok;
	for (int i = 0; i < 12; i++) {
		tok = t.ReadNextToken();
		printf("%d ", tok.u.number);
	}
	tok = t.ReadNextToken();
	printf("%s\n", tok.u.string);
}


bool
Parser::_FindEndOfBlock(token blockHead, const uint32 &maxEnd, uint32 &position)
{
	bool endBlock = false;
	while ((uint32)fStream->Position() < maxEnd) {
		if (fTokenizer->ReadNextToken() == blockHead) {
			endBlock = true;
			break;
		}
	}

	position = fStream->Position() - 2;
	return endBlock;
}


void
Parser::_SkipUselessTokens()
{
	std::string skipped;
	token tok;
	for (;;) {
		tok = fTokenizer->ReadNextToken();
		if (tok.type != TOKEN_SPACE
			&& tok.type != TOKEN_END_OF_LINE
			&& tok.type != TOKEN_UNKNOWN) {
			break;
		}
		skipped.append(tok.u.string);
	}

	fTokenizer->RewindToken(&tok);
}




void
Parser::_ReadElementGuard(node* n)
{
	_SkipUselessTokens();
	token tok = fTokenizer->ReadNextToken();
	if (tok.type == TOKEN_BLOCK_GUARD) {
		int blockType = Parser::_BlockTypeFromToken(tok);
		if (!n->closed && blockType == n->type) {
			n->closed = true;
		} else {
			n->type = blockType;
			strcpy(n->header, tok.u.string);
		}
	} else
		fTokenizer->RewindToken(&tok);
}


void
Parser::_ReadElementValue(node* n)
{
	_ReadElementGuard(n);
	for (;;) {
		_SkipUselessTokens();
		token tok = fTokenizer->ReadNextToken();
		if (tok.type == TOKEN_BLOCK_GUARD) {
			fTokenizer->RewindToken(&tok);
			if (Parser::_BlockTypeFromToken(tok) == n->type) {
				// Means the block is open, and this is the closure.
				// Bail out and let _ReadElementGuard to the rest.
				break;
			} else {
				// We found a nested block,
				node *newNode = new node;
				try {
					_ReadElementValue(newNode);
				} catch (...) {

				}
				n->AddChild(newNode);
			}
		} else {
			if (n->type == BLOCK_TRIGGER) {
				// TODO:
			}
			// TODO: Should read the value and store it correctly
			if (tok.type == TOKEN_STRING || tok.type == TOKEN_NUMBER) {
				strcat(n->value, " ");
				strcat(n->value, tok.u.string);
			}
		}
	}

	_ReadElementGuard(n);
}


int
Parser::_BlockTypeFromToken(const token& tok)
{
	if (tok == token("SC"))
		return BLOCK_SCRIPT;
	else if (tok == token("CR"))
		return BLOCK_CONDITION_RESPONSE;
	else if (tok == token("CO"))
		return BLOCK_CONDITION;
	else if (tok == token("TR"))
		return BLOCK_TRIGGER;
	else if (tok == token("OB"))
		return BLOCK_OBJECT;
	else if (tok == token("RE"))
		return BLOCK_REEESPONSE;
	else if (tok == token("RS"))
		return BLOCK_RESPONSE;
	else if (tok == token("AC"))
		return BLOCK_ACTION;


	throw "Block Error!";

	return -1;
}


// Tokenizer
Tokenizer::Tokenizer()
	:
	fStream(NULL),
	fPosition(0)
{
}


Tokenizer::Tokenizer(Stream *stream, int32 position)
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


token
Tokenizer::ReadToken()
{
	return ReadNextToken();
}


token
Tokenizer::ReadNextToken()
{
	int32 startToken = fStream->Position();

	for (;;) {
		char c = fStream->ReadByte();
		if (IsSeparator(c))
			break;
	}
	int32 endToken = fStream->Position() - 1;
	if (endToken == startToken)
		endToken++;
	int32 supposedSize = endToken - startToken;
	char array[supposedSize + 1];
	fStream->ReadAt(startToken, array, supposedSize);
	array[supposedSize] = '\0';
	token aToken;
	switch (array[0]) {
		case 'S':
		{
			if (array[1] == 'C') {
				aToken.type = TOKEN_BLOCK_GUARD; // script
				aToken.size = 2;
				memcpy(aToken.u.string, array, aToken.size);
				aToken.u.string[aToken.size] = '\0';
			}
			break;
		}
		case 'C':
		{
			if (array[1] == 'R' || array[1] == 'O') {
				aToken.type = TOKEN_BLOCK_GUARD; // condition or condition-response
				aToken.size = 2;
				memcpy(aToken.u.string, array, aToken.size);
				aToken.u.string[aToken.size] = '\0';
			}
			break;
		}

		case 'T':
		{
			if (array[1] == 'R') {
				aToken.type = TOKEN_BLOCK_GUARD; // trigger
				aToken.size = 2;
				memcpy(aToken.u.string, array, aToken.size);
				aToken.u.string[aToken.size] = '\0';
			}
			break;
		}
		case 'O':
		{
			if (array[1] == 'B') {
				aToken.type = TOKEN_BLOCK_GUARD; // object
				aToken.size = 2;
				memcpy(aToken.u.string, array, aToken.size);
				aToken.u.string[aToken.size] = '\0';
			}
			break;
		}
		case 'R':
		{
			if (array[1] == 'E' || array[1] == 'S') {
				aToken.type = TOKEN_BLOCK_GUARD; // response
				aToken.size = 2;
				memcpy(aToken.u.string, array, aToken.size);
				aToken.u.string[aToken.size] = '\0';
			}
			break;
		}
		case 'A':
		{
			if (array[1] == 'C') {
				aToken.type = TOKEN_BLOCK_GUARD; // action
				aToken.size = 2;
				memcpy(aToken.u.string, array, aToken.size);
				aToken.u.string[aToken.size] = '\0';
			}
			break;
		}
		case '\n':
		case '\r':
			aToken.type = TOKEN_END_OF_LINE;
			strcpy(aToken.u.string, "\\ENDL\\");
			aToken.size = 1;
			break;
		case ' ':
			aToken.type = TOKEN_SPACE;
			aToken.u.string[0] = ' ';
			aToken.u.string[1] = '\0';
			aToken.size = 1;
			break;
		case '"':
		{
			aToken.type = TOKEN_STRING;
			aToken.size = strrchr(array, '"') - array + 1;
			memcpy(aToken.u.string, array, aToken.size);
			aToken.u.string[aToken.size] = '\0';
			break;
		}
		default:
		{
			aToken.type = TOKEN_NUMBER;
			aToken.size = supposedSize;
			char* rest = NULL;
			aToken.u.number = strtol(array, &rest, 0);
			if (rest != NULL)
				aToken.size = std::min(rest - array, aToken.size);
			memcpy(aToken.u.string, array, aToken.size);
			aToken.u.string[aToken.size] = '\0';

			break;
		}
	}

	// We could have read too much, rewind a bit if needed
	fStream->Seek(startToken + aToken.size, SEEK_SET);
	return aToken;
}


void
Tokenizer::RewindToken(token *tok)
{
	if (tok->size != 0)
		fStream->Seek(-tok->size, SEEK_CUR);
}


/* static */
bool
Tokenizer::IsSeparator(char const &c)
{
	return (c == ' ' || c == '\n' || c == '\r');
}
