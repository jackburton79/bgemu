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
Parser::PrintNode(node* n) const
{
	PrintIndentation();
	printf("<%s>", n->header);
	if (n->type == BLOCK_TRIGGER) {
		trigger* tr = dynamic_cast<trigger*>(n);
		printf(" id:%d, flags: %d, %s %s\n",
			tr->id, tr->flags, tr->string1, tr->string2);
	} else
		printf(" %s\n", n->value);
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
Parser::Read(node*& rootNode)
{
	try {
		_ReadElement(rootNode);
	} catch (const char *str) {
		printf("caught string %s\n", str);
	} catch (...) {
		printf("end of file!\n");
	}

	PrintNode(rootNode);
}


/* static */
node*
Parser::CreateNode(int type, const char *string)
{
	node* newNode = NULL;
	switch (type) {
		case BLOCK_TRIGGER:
			newNode = new trigger;
			break;
		/*case BLOCK_OBJECT:
			newNode = new object;
			break;*/
		default:
			newNode = new node;
			break;
	}
	if (newNode != NULL) {
		newNode->type = type;
		strcpy(newNode->header, string);
	}
	return newNode;
}


void
Parser::_ReadTriggerBlock(::node* node)
{
	trigger* trig = dynamic_cast<trigger*>(node);
	trig->id = fTokenizer->ReadToken().u.number;
	printf("id: %d\n", trig->id);
	trig->parameter1 = fTokenizer->ReadToken().u.number;
	printf("parameter1: %d\n", trig->parameter1);
	trig->flags = fTokenizer->ReadToken().u.number;
	printf("flags: %d\n", trig->flags);
	trig->parameter2 = fTokenizer->ReadToken().u.number;
	printf("parameter2: %d\n", trig->parameter2);
	trig->unknown = fTokenizer->ReadToken().u.number;
	printf("unknown: %d\n", trig->unknown);
	strcpy(trig->string1, fTokenizer->ReadToken().u.string);
	printf("string1: %s\n", trig->string1);
	strcpy(trig->string2, fTokenizer->ReadToken().u.string);
	printf("string1: %s\n", trig->string2);
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


void
Parser::_ReadElementGuard(node*& n)
{
	token tok = fTokenizer->ReadNextToken();
	if (tok.type == TOKEN_BLOCK_GUARD) {
		int blockType = Parser::_BlockTypeFromToken(tok);
		if (n == NULL)
			n = Parser::CreateNode(blockType, tok.u.string);
		else if (!n->closed && blockType == n->type) {
			n->closed = true;
		}
	} else
		fTokenizer->RewindToken(tok);
}


void
Parser::_ReadElement(::node*& node)
{
	_ReadElementGuard(node);
	for (;;) {
		token tok = fTokenizer->ReadNextToken();
		if (tok.type == TOKEN_BLOCK_GUARD) {
			fTokenizer->RewindToken(tok);
			if (Parser::_BlockTypeFromToken(tok) == node->type) {
				// Means the block is open, and this is the closure.
				// Bail out and let _ReadElementGuard to the rest.
				break;
			} else {
				// We found a nested block,
				::node *newNode = NULL;
				try {
					_ReadElement(newNode);
				} catch (...) {

				}
				node->AddChild(newNode);
			}
		} else {
			_ReadElementValue(node, tok);
		}
	}

	_ReadElementGuard(node);
}


void
Parser::_ReadElementValue(::node* node, const token& tok)
{
	if (node->type == BLOCK_TRIGGER) {
		// TODO: For now, since _ReadTriggerBlock()
		// wants to read the whole block
		fTokenizer->RewindToken(tok);
		_ReadTriggerBlock(node);

	} else {
		// TODO: Should read the value and store it correctly
		if (tok.type == TOKEN_STRING) {
			strcat(node->value, " ");
			strcat(node->value, tok.u.string);
		} else if (tok.type == TOKEN_NUMBER) {
			char numb[16];
			snprintf(numb, sizeof(numb), "%d", tok.u.number);
			strcat(node->value, " ");
			strcat(node->value, numb);
		}
	}
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
	_SkipSeparators();

	char array[128];
	int32 startToken = fStream->Position();
	int32 supposedSize = _ReadFullToken(array, startToken);
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
			break;
		}
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
Tokenizer::_SkipSeparators()
{
	for (;;) {
		char c = fStream->ReadByte();
		if (!Tokenizer::IsSeparator(c))
			break;
	}

	fStream->Seek(-1, SEEK_CUR);
}


int32
Tokenizer::_ReadFullToken(char *dest, int32 start)
{
	char *ptr = dest;
	for (;;) {
		char c = fStream->ReadByte();
		if (Tokenizer::IsSeparator(c))
			break;
		*ptr++ = c;
	}

	return fStream->Position() - 1 - start;
}


/* static */
bool
Tokenizer::IsSeparator(char const &c)
{
	return (c == ' ' || c == '\n' || c == '\r');
}
