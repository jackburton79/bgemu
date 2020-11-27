#include "Core.h"
#include "Parsing.h"
#include "ResManager.h"
#include "Script.h"
#include "StringStream.h"
#include "Triggers.h"

#include <algorithm>
#include <assert.h>
#include <ctype.h>
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
	:
	type(TOKEN_UNKNOWN),
	size(0)
{
	strncpy(u.string, tok, sizeof(u.string));
	size = strlen(u.string);

	if (!memcmp(tok, "SC", 2)
		|| !memcmp(tok, "CR", 2)
		|| !memcmp(tok, "CO", 2)
		|| !memcmp(tok, "TR", 2)
		|| !memcmp(tok, "OB", 2)
		|| !memcmp(tok, "RS", 2)
		|| !memcmp(tok, "RE", 2)
		|| !memcmp(tok, "AC", 2))
		type = TOKEN_STRING;
	else if (tok[0] == ' ')
		type = TOKEN_SPACE;
	else if (tok[0] == '\n' || tok[0] == '\r')
		type = TOKEN_END_OF_LINE;
	else if (tok[0] == '"')
		type = TOKEN_QUOTED_STRING;
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


/* static */
std::vector<trigger_node>
Parser::TriggersFromString(const std::string& string)
{
	std::string localString = string;
	std::vector<trigger_node> triggerList;
	while (true) {
		trigger_node triggerNode;
		if (TriggerFromString(localString, triggerNode))
			triggerList.push_back(triggerNode);
		size_t endLine = localString.find('\n');
		if (endLine == localString.length() || endLine == std::string::npos)
			break;

		localString = localString.substr(endLine + 1, string.length());
	}
	return triggerList;
}


/* static */
bool
Parser::TriggerFromString(const std::string& string, trigger_node& node)
{
	node.type = BLOCK_TRIGGER;
	StringStream stream(string);
	Tokenizer tokenizer(&stream, 0);

	if (!_ExtractTriggerName(tokenizer, &node))
		return false;
	_ExtractFirstParameter(tokenizer, &node);
	_ExtractSecondParameter(tokenizer, &node);

	node.Print();
	return true;
}


void
Parser::Read(node*& rootNode)
{
	try {
		_ReadNode(rootNode);
	} catch (const char *str) {
		std::cerr << "Parser::Read(): caught string " << str << std::endl;
	} catch (...) {
		std::cerr << "Parser::Read(): end of file!" << std::endl;
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

		// We remove the "", that's why we start from token.u.string + 1 and copy size - 2
		token stringToken = tokenizer->ReadToken();
		size_t newTokenSize = 0;
		if (stringToken.size > 2) {
			// less than 2 means it's an empty string, like ""
			newTokenSize = stringToken.size - 2;
			strncpy(trig->string1, stringToken.u.string + 1, newTokenSize);
		}
		trig->string1[newTokenSize] = '\0';

		token stringToken2 = tokenizer->ReadToken();
		size_t newTokenSize2 = 0;
		if (stringToken2.size > 2) {
			newTokenSize2 = stringToken2.size - 2;
			strncpy(trig->string2, stringToken2.u.string + 1, newTokenSize2);
		}
		trig->string2[newTokenSize2] = '\0';
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
		token stringToken = tokenizer->ReadNextToken();
		char* name = stringToken.u.string;
		char* nameEnd = name + stringToken.size;
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
		token stringToken = tokenizer->ReadToken();
		strncpy(act->string1, stringToken.u.string + 1, stringToken.size - 2);
		act->string1[stringToken.size - 2] = '\0';
		token stringToken2 = tokenizer->ReadToken();
		strncpy(act->string2, stringToken2.u.string + 1, stringToken2.size - 2);
		act->string2[stringToken2.size - 2] = '\0';
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


/* static */
bool
Parser::_ExtractTriggerName(Tokenizer& tokenizer, ::trigger_node* node)
{
	// Trigger name and modifier
	token t = tokenizer.ReadToken();
	if (t.type == TOKEN_EXCLAMATION_MARK) {
		node->flags = 1;
		t = tokenizer.ReadToken();
	}
	if (t.type != TOKEN_STRING)
		return false;
	std::string triggerName = t.u.string;
	try {
		node->id = GetTriggerID(triggerName);
	} catch (std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		return false;
	}
	return true;
}


/* static */
void
Parser::_ExtractFirstParameter(Tokenizer& tokenizer, ::trigger_node* node)
{
	tokenizer.ReadToken(); // parenthesis

	token t = tokenizer.ReadToken();
	if (t.type == TOKEN_NUMBER)
		node->parameter1 = t.u.number;
	else {
		// Unquote string
		strncpy(node->string1, t.u.string + 1, strlen(t.u.string) - 1);
		node->string1[strlen(t.u.string) - 2] = '\0';
	}
}


/* static */
void
Parser::_ExtractSecondParameter(Tokenizer& tokenizer, ::trigger_node* node)
{
	token t = tokenizer.ReadToken();
	if (t.type != TOKEN_PARENTHESIS) {
		if (t.type == TOKEN_COMMA)
			t = tokenizer.ReadToken();
		if (t.type == TOKEN_NUMBER)
			node->parameter2 = t.u.number;
		else {
			// Unquote string
			strncpy(node->string2, t.u.string + 1, strlen(t.u.string) - 1);
			node->string2[strlen(t.u.string) - 2] = '\0';
		}
	}
}


/* static */
void
Parser:: _ExtractThirdParameter(Tokenizer& tokenizer, ::trigger_node* triggerNode)
{
	// TODO:
}


void
Parser::_ReadNodeHeader(node*& n)
{
	token tok = fTokenizer->ReadNextToken();
	int blockType = Parser::_BlockTypeFromToken(tok);
	if (n == NULL)
		n = node::Create(blockType, tok.u.string);
	else if (!n->closed && blockType == n->type) {
		n->closed = true;
	}
}


void
Parser::_ReadNode(::node*& node)
{
	_ReadNodeHeader(node);
	for (;;) {
		token tok = fTokenizer->ReadNextToken();
		int blockType =_BlockTypeFromToken(tok);
		if (blockType != -1) {
			fTokenizer->RewindToken(tok);
			if (blockType == node->type) {
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
					_ReadNode(newNode);
				} catch (...) {
					// Finished
				}
				node->AddChild(newNode);
			}
		} else {
			_ReadNodeValue(node, tok);
		}
	}

	_ReadNodeHeader(node);

	if (fDebug)
		node->Print();
}


void
Parser::_ReadNodeValue(::node* node, const token& tok)
{
	if (node->value[0] != '\0')
		strcat(node->value, " ");

	if (tok.type == TOKEN_QUOTED_STRING) {
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

	// token is not a header guard
	// so we cannot gues the block type
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
		case TOKEN_PARENTHESIS:
			return "TOKEN_PARENTHESIS";
		default:
			return "TOKEN_UNKNOWN";
	}
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
	} else if (array[0] == '(' || array[0] == ')') {
		aToken.type = TOKEN_PARENTHESIS;
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
	bool quotesOpen = false;
	char* ptr = dest;
	try {
		for (;;) {
			char c = fStream->ReadByte();
			if (Tokenizer::IsSeparator(c)) {
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
	} catch (...) {

	}
	return fStream->Position() - start;
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
}


bool
operator==(const node &a, const node &b)
{
	return false;
}


// trigger
trigger_node::trigger_node()
	:
	id(0),
	parameter1(0),
	flags(0),
	parameter2(0),
	unknown(0)
{
	string1[0] = '\0';
	string2[0] = '\0';
}


void
trigger_node::Print() const
{
	std::cout << IDTable::TriggerName(id);
	std::cout << "(" << std::dec << (int)id << ", 0x" << std::hex << (int)id;
	std::cout << "), flags: " << std::dec << flags << ", parameter1:";
	std::cout << std::dec;
	std::cout << parameter1 << ", parameter2: " << parameter2 << ", string1: " << string1;
	std::cout << ", string2: " << string2 << std::endl;
}


object_node*
trigger_node::Object()
{
	return (object_node*)children.front();
}


// object
object_node::object_node()
	:
	team(0),
	faction(0),
	ea(0),
	general(0),
	race(0),
	classs(0),
	specific(0),
	gender(0),
	alignment(0)
{
	memset(identifiers, 0, sizeof(identifiers));
	point.x = point.y = -1;
	name[0] = '\0';
}


void
object_node::Print() const
{
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
		std::cout << "alignment: " << IDTable::AlignmentAt(alignment) << " (" << alignment << "), ";
	for (int32 i = 4; i >= 0; i--) {
		if (identifiers[i] != 0) {
			std::cout << IDTable::ObjectAt(identifiers[i]);
			if (i != 0)
				std::cout << " -> ";
		}
	}
	if (Core::Get()->Game() == GAME_TORMENT)
		std::cout << "point: " << point.x << ", " << point.y << ", ";
	if (name[0] != '\0')
		std::cout << "name: *" << name << "*" << ", ";
	if (Empty())
		std::cout << "EMPTY (MYSELF)";
	std::cout << std::endl;
}


bool
object_node::Empty() const
{
	if (ea == 0
			&& general == 0
			&& race == 0
			&& classs == 0
			&& specific == 0
			&& gender == 0
			&& alignment == 0
			//&& faction == 0
			//&& team == 0
			&& identifiers[0] == 0
			&& identifiers[1] == 0
			&& identifiers[2] == 0
			&& identifiers[3] == 0
			&& identifiers[4] == 0
			&& name[0] == '\0'
			) {
		return true;
	}

	return false;
}


// action
action_node::action_node()
	:
	id(0),
	integer1(0),
	integer2(0),
	integer3(0)
{
	where.x = where.y = -1;
	string1[0] = '\0';
	string2[0] = '\0';
}


void
action_node::Print() const
{
	std::cout << IDTable::ActionAt(id);
	std::cout << "(" << std::dec << (int)id << std::hex << ", 0x" << (int)id << ")";
	std::cout << std::dec;
	std::cout << " ";
	std::cout << "integer1: " << integer1 << ", ";
	std::cout << "where: (" << where.x << ", " << where.y << "), ";
	std::cout << "integer2: " << integer2 << ", ";
	std::cout << "integer3: " << integer3 << ", ";
	std::cout << "string1: " << string1 << ", ";
	std::cout << "string2: " << string2 << std::endl;
}


object_node*
action_node::First()
{
	return (object_node*)(*children.begin());
}


object_node*
action_node::Second()
{
	node_list::iterator i = children.begin();
	i++;

	return (object_node*)(*i);
}


object_node*
action_node::Third()
{
	node_list::iterator i = children.begin();
	i++;
	i++;

	return (object_node*)(*i);
}


//response
response_node::response_node()
{
}


void
response_node::Print() const
{
	std::cout << "probability: " << probability << std::endl;
}
