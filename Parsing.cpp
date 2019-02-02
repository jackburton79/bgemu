#include "Core.h"
#include "Parsing.h"
#include "ResManager.h"
#include "Script.h"
#include "StringStream.h"

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


// Parser
Parser::Parser()
	:
	fStream(NULL),
	fTokenizer(NULL),
	fDebug(false)
{
	fTokenizer = new Tokenizer();
}


Parser::~Parser()
{
	delete fTokenizer;
}


void
Parser::SetTo(Stream *stream)
{
	fStream = stream;
	fStream->Seek(0, SEEK_SET);

	fTokenizer->SetTo(stream, 0);
}


void
Parser::SetDebug(bool debug)
{
	fDebug = debug;
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
}


/* static */
void
Parser::_ReadTriggerBlock(Tokenizer *tokenizer,::node* node)
{
	trigger_node* trig = dynamic_cast<trigger_node*>(node);
	if (trig) {
		trig->id = tokenizer->ReadToken().u.number;
		trig->parameter1 = tokenizer->ReadToken().u.number;
		trig->flags = tokenizer->ReadToken().u.number;
		trig->parameter2 = tokenizer->ReadToken().u.number;
		trig->unknown = tokenizer->ReadToken().u.number;
		strcpy(trig->string1, tokenizer->ReadToken().u.string + 1);
		trig->string1[strlen(trig->string1) - 1] = '\0';
		strcpy(trig->string2, tokenizer->ReadToken().u.string + 1);
		trig->string2[strlen(trig->string2) - 1] = '\0';
	}
}


/* static */
void
Parser::_ReadObjectBlock(Tokenizer *tokenizer, ::node* node)
{
	object_node* obj = dynamic_cast<object_node*>(node);
	if (obj) {
		obj->ea = tokenizer->ReadNextToken().u.number;
		if (Core::Get()->Game() == GAME_TORMENT) {
			obj->faction = tokenizer->ReadNextToken().u.number;
			obj->team = tokenizer->ReadNextToken().u.number;
		}
		obj->general = tokenizer->ReadNextToken().u.number;
		obj->race = tokenizer->ReadNextToken().u.number;
		obj->classs = tokenizer->ReadNextToken().u.number;
		obj->specific = tokenizer->ReadNextToken().u.number;
		obj->gender = tokenizer->ReadNextToken().u.number;
		obj->alignment = tokenizer->ReadNextToken().u.number;
		for (int32 i = 0; i < 5; i++)
			obj->identifiers[i] = tokenizer->ReadNextToken().u.number;

		// TODO: Not sure which games supports that
		if (Core::Get()->Game() == GAME_TORMENT) {
			obj->point.x = tokenizer->ReadNextToken().u.number;
			obj->point.y = tokenizer->ReadNextToken().u.number;
		}

		// Remove the quotation marks
		char* name = tokenizer->ReadNextToken().u.string;
		char* nameEnd = name + strlen(name);
		while (*name == '"')
			name++;
		while (*nameEnd != '"')
			nameEnd--;
		*nameEnd = '\0';
		strcpy(obj->name, name);
	}
}


/* static */
void
Parser::_ReadActionBlock(Tokenizer *tokenizer, node* node)
{
	action_node* act = dynamic_cast<action_node*>(node);
	if (act) {
		act->id = tokenizer->ReadNextToken().u.number;
		act->integer1 = tokenizer->ReadNextToken().u.number;
		act->where.x = tokenizer->ReadNextToken().u.number;
		act->where.y = tokenizer->ReadNextToken().u.number;
		act->integer2 = tokenizer->ReadNextToken().u.number;
		act->integer3 = tokenizer->ReadNextToken().u.number;
		// TODO: This removes "" from strings. Broken.
		// Should do this from the beginning!!!
		strcpy(act->string1, tokenizer->ReadToken().u.string + 1);
		act->string1[strlen(act->string1) - 1] = '\0';
		strcpy(act->string2, tokenizer->ReadToken().u.string + 1);
		act->string2[strlen(act->string2) - 1] = '\0';
	}
}


/* static */
void
Parser::_ReadResponseBlock(Tokenizer *tokenizer, node* node)
{
	response_node* resp = dynamic_cast<response_node*>(node);
	if (resp)
		resp->probability = tokenizer->ReadNextToken().u.number;
}


void
Parser::_ReadElementGuard(node*& n)
{
	token tok = fTokenizer->ReadNextToken();
	if (tok.type == TOKEN_BLOCK_GUARD) {
		int blockType = Parser::_BlockTypeFromToken(tok);
		if (n == NULL)
			n = node::Create(blockType, tok.u.string);
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
				// Means the block is open, and this is
				// the closing tag. FixNode will copy the node values
				// to the node specific values.
				// _ReadElementGuard will do the rest.
				_FixNode(node);
				break;
			} else {
				// We found a nested block,
				::node *newNode = NULL;
				try {
					_ReadElement(newNode);
				} catch (...) {
					// Finished
				}
				node->AddChild(newNode);
			}
		} else {
			_ReadElementValue(node, tok);
		}
	}

	_ReadElementGuard(node);

	if (fDebug)
		node->Print();
}


void
Parser::_ReadElementValue(::node* node, const token& tok)
{
	if (strlen(node->value) > 0)
		strcat(node->value, " ");

	if (tok.type == TOKEN_STRING) {
		strcat(node->value, tok.u.string);
	} else if (tok.type == TOKEN_NUMBER) {
		char numb[16];
		snprintf(numb, sizeof(numb), "%d", tok.u.number);
		strcat(node->value, numb);
	}
}


void
Parser::_FixNode(::node* node)
{
	StringStream stream(node->value);
	Tokenizer tokenizer(&stream, 0);
	//tokenizer.SetDebug(true);
	switch (node->type) {
		case BLOCK_TRIGGER:
			_ReadTriggerBlock(&tokenizer, node);
			break;
		case BLOCK_ACTION:
			_ReadActionBlock(&tokenizer, node);
			break;
		case BLOCK_OBJECT:
			_ReadObjectBlock(&tokenizer, node);
			break;
		case BLOCK_RESPONSE:
			_ReadResponseBlock(&tokenizer, node);
			break;
		default:
			break;
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
		return BLOCK_RESPONSE;
	else if (tok == token("RS"))
		return BLOCK_RESPONSESET;
	else if (tok == token("AC"))
		return BLOCK_ACTION;

	throw "Unknown block!";

	return -1;
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
				aToken.size = std::min((int)(rest - array), aToken.size);
			break;
		}
	}

	if (fDebug) {
		printf("token: type %d, value ", aToken.type);
		if (aToken.type == TOKEN_NUMBER)
			printf("%d", aToken.u.number);
		else
			printf("%s", aToken.u.string);
		printf("\n");
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
	for (;;) {
		char c = fStream->ReadByte();
		if (!Tokenizer::IsSeparator(c))
			break;
	}

	fStream->Seek(-1, SEEK_CUR);
}


int32
Tokenizer::_ReadFullToken(char* dest, int32 start)
{
	try {
		char* ptr = dest;
		for (;;) {
			char c = fStream->ReadByte();
			if (Tokenizer::IsSeparator(c))
				break;
			*ptr++ = c;
		}
	} catch (...) {
		//printf("end of stream exception\n");
	}
	return fStream->Position() - 1 - start;
}


/* static */
bool
Tokenizer::IsSeparator(char const &c)
{
	return (c == ' ' || c == '\n' || c == '\r' || c == '\t');
}


// node
/* static */
node*
node::Create(int type, const char *string)
{
	node* newNode = NULL;
	switch (type) {
		case BLOCK_TRIGGER:
			newNode = new trigger_node;
			break;
		case BLOCK_OBJECT:
			newNode = new object_node;
			break;
		case BLOCK_ACTION:
			newNode = new action_node;
			break;
		case BLOCK_RESPONSE:
			newNode = new response_node;
			break;
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


// node
node::node()
	:
	type(BLOCK_UNKNOWN),
	parent(NULL),
	next(NULL),
	closed(false)

{
	value[0] = '\0';
}


node::~node()
{
	node_list::iterator i;
	for (i = children.begin(); i != children.end(); i++)
		delete (*i);
}


void
node::AddChild(node* child)
{
	child->parent = this;
	child->next = NULL;
	if (children.size() > 0) {
		std::vector<node*>::reverse_iterator i = children.rbegin();
		(*i)->next = child;
	}
	children.push_back(child);
}


node*
node::Next() const
{
	return next;
}


void
node::Print() const
{
	//printf("header: %s\n", header);
	//printf("value: %s\n", value);
}


bool
operator==(const node &a, const node &b)
{
	return false;
}


// trigger
trigger_node::trigger_node()
{
}


void
trigger_node::Print() const
{
	printf("id:%d, flags: %d, parameter1: %d, parameter2: %d, %s %s\n",
		id, flags, parameter1, parameter2, string1, string2);
}


// object
object_node::object_node()
{
}


void
object_node::Print() const
{
	std::cout << "Object: ";
	if (Core::Get()->Game() == GAME_TORMENT) {
		std::cout << "team: " << team << ", ";
		std::cout << "faction: " << faction << ", ";
	}
	if (ea != 0)
		std::cout << "ea: " << IDTable::EnemyAllyAt(ea) << " (" << ea << "), ";
	if (general != 0)
		std::cout << "general: " << IDTable::GeneralAt(general) << " (" << general << "), ";
	if (race != 0)
		std::cout << "race: " << IDTable::RaceAt(race) << " (" << race << "), ";
	if (classs != 0)
		std::cout << "class: " << IDTable::ClassAt(classs) << " (" << classs << "), ";
	if (specific != 0)
		std::cout << "specific: " << IDTable::SpecificAt(specific) << " (" << specific << "), ";
	if (gender != 0)
		std::cout << "gender: " << IDTable::GenderAt(gender) << " (" << gender << "), ";
	if (alignment != 0)
		std::cout << "alignment: " << IDTable::AlignmentAt(alignment) << " (" << alignment << "), " << std::endl;
	for (int32 i = 4; i >= 0; i--) {
		if (identifiers[i] != 0) {
			std::cout << IDTable::ObjectAt(identifiers[i]);
			if (i != 0)
				std::cout << " -> ";
		}
	}
	std::cout << std::endl;
	if (Core::Get()->Game() == GAME_TORMENT)
		std::cout << "point: " << point.x << ", " << point.y << std::endl;
	if (name[0] != '\0')
		 std::cout << "name: *" << name << "*" << std::endl;
}


// action
action_node::action_node()
{
}


void
action_node::Print() const
{
	printf("SCRIPT: id: %d, parameter: %d, point: (%d, %d), %d, %d, %s, %s\n",
			id, integer1, where.x, where.y, integer2, integer3, string1, string2);
}


//response
response_node::response_node()
{
}


void
response_node::Print() const
{
	printf("probability: %d\n", probability);
}
