#include "Parsing.h"

#include "Core.h"
#include "IDSResource.h"
#include "Log.h"
#include "ResManager.h"
#include "Script.h"
#include "StringStream.h"
#include "Triggers.h"
#include "Utils.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <ctype.h>
#include <string>


class Parameter {
public:
	Parameter();
	std::string Name() const;
	std::string Type() const;

	void Print() const;

	enum {
		INTEGER,
		INT_ENUM,
		STRING,
		OBJECT,
		POINT,
		UNKNOWN
	};
	std::string name;
	int type;
	int position;
	std::string IDtable;
};


class TriggerParameters {
	std::vector<Parameter> parameters;
};


class ParameterExtractor {
public:
	ParameterExtractor(Tokenizer& tokenizer);
	token _ExtractNextParameter(::trigger_params* triggerNode,
								Parameter& parameter);
private:
	Tokenizer& fTokenizer;
};


Parameter::Parameter()
	:
	type(UNKNOWN),
	position(1)
{
}


std::string
Parameter::Name() const
{
	return name;
}


std::string
Parameter::Type() const
{
	switch (type) {
		case OBJECT:
			return "OBJECT";
		case INTEGER:
			return "INTEGER";
		case STRING:
			return "STRING";
		case INT_ENUM:
			return "INT_ENUM";
		case POINT:
			return "POINT";
		case UNKNOWN:
		default:
			return "UNKNOWN";
	}
}

void
Parameter::Print() const
{
	std::cout << "name:" << Name() << std::endl;
	std::cout << "type:" << Type() << std::endl;
	std::cout << "position: " << position << std::endl;
	if (!IDtable.empty())
		std::cout << "IDtable: " << IDtable << std::endl;
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
std::vector<trigger_params*>
Parser::TriggersFromString(const std::string& string)
{
	std::string localString = string;
	std::vector<trigger_params*> triggerList;
	if (!string.empty()) {
		while (true) {
			trigger_params* triggerNode = TriggerFromString(localString);
			if (triggerNode != NULL)
				triggerList.push_back(triggerNode);
			size_t endLine = localString.find('\n');
			if (endLine == localString.length() || endLine == std::string::npos)
				break;
			localString = localString.substr(endLine + 1, string.length());
		}
	}
	return triggerList;
}


static
Parameter
ParameterFromString(const std::string& string, int& stringPos, int& integerPos)
{
	Parameter parameter;
	std::string typeString = string.substr(0, 2);
	size_t valueNamePos = string.find(":");
	size_t IDSNamePos = string.find("*");
	std::string valueName = string.substr(valueNamePos + 1, IDSNamePos - 2);
	std::string valueIDS = string.substr(IDSNamePos + 1, std::string::npos);
	parameter.name = valueName;
	if (typeString == "O:") {
		parameter.type = Parameter::OBJECT;
	} else if (typeString == "S:") {
		parameter.type = Parameter::STRING;
		parameter.position = stringPos;
		stringPos++;
	} else if (typeString == "I:") {
		if (valueIDS == "")
			parameter.type = Parameter::INTEGER;
		else {
			parameter.type = Parameter::INT_ENUM;
			parameter.IDtable = valueIDS;
		}
		parameter.position = integerPos;
		integerPos++;
	}
	return parameter;
}


static
std::vector<Parameter>
GetFunctionParameters(std::string functionString)
{
	std::cout << "GetFunctionParameters()" << std::endl;
	StringStream stream(functionString);
	Tokenizer tokenizer(&stream, 0);
	//tokenizer.SetDebug(true);

	std::vector<Parameter> parameters;
	token functionName = tokenizer.ReadToken();
	token parens = tokenizer.ReadToken();
	if (functionName.type != TOKEN_STRING
			|| parens.type != TOKEN_PARENTHESIS_OPEN)
		return parameters;

	// TODO: Improve, refactor
	int stringPos = 1;
	int integerPos = 1;
	for (;;) {
		token t = tokenizer.ReadToken();
		// closing parenthesis
		if (t.type == TOKEN_PARENTHESIS_CLOSED)
			break;
		else if (t.type == TOKEN_COMMA)
			continue;
		Parameter parameter = ParameterFromString(t.u.string, stringPos, integerPos);
		parameters.push_back(parameter);
	}

	std::cout << "found " << parameters.size() << " parameters." << std::endl;
	std::vector<Parameter>::const_iterator i;
	for (i = parameters.begin(); i != parameters.end(); i++) {
		(*i).Print();
	}

	return parameters;
}


/* static */
trigger_params*
Parser::TriggerFromString(const std::string& string)
{
	std::cout << "TriggerFromString()" << std::endl;
	trigger_params* node = new trigger_params();
	node->type = BLOCK_TRIGGER;
	StringStream stream(string);
	Tokenizer tokenizer(&stream, 0);
	//tokenizer.SetDebug(true);
	if (!_ExtractTriggerName(tokenizer, node)) {
		delete node;
		return NULL;
	}

	// Opening parenthesis
	try {
		token parenthesis = tokenizer.ReadToken();
		assert(parenthesis.type == TOKEN_PARENTHESIS_OPEN);
	} catch (std::exception& e)	{
		std::cerr << Log::Yellow << e.what() << Log::Normal << std::endl;
		return NULL;
	}
	ParameterExtractor extractor(tokenizer);
	std::vector<Parameter> paramTypes = GetFunctionParameters(IDTable::TriggerName(node->id));
	for (std::vector<Parameter>::const_iterator i = paramTypes.begin();
			i != paramTypes.end(); i++) {
		Parameter parameter = *i;
		extractor._ExtractNextParameter(node, parameter);
	}

	std::cout << "TriggerFromString() END" << std::endl;
	node->Print();

	return node;
}


/* static */
bool
Parser::ActionFromString(const std::string& string, action_params& node)
{
	return false;
}


node*
Parser::Read()
{
	node* rootNode = NULL;
	try {
		// allocates the node
		_ReadNode(rootNode);
	} catch (std::exception& except) {
		std::cerr << Log::Red << "Parser::Read(): " << except.what() << std::endl;
	} catch (...) {
		std::cerr << Log::Red << "Parser::Read(): unknown exception" << std::endl;
	}
	return rootNode;
}


/* static */
void
Parser::_ReadTriggerBlock(Tokenizer *tokenizer,::node* node)
{
	trigger_params* trig = dynamic_cast<trigger_params*>(node);
	if (trig) {
		trig->id = tokenizer->ReadToken().u.number;
		trig->parameter1 = tokenizer->ReadToken().u.number;
		trig->flags = tokenizer->ReadToken().u.number;
		trig->parameter2 = tokenizer->ReadToken().u.number;
		trig->unknown = tokenizer->ReadToken().u.number;

		// Strings are quoted. We remove quotes
		token stringToken = tokenizer->ReadToken();
		get_unquoted_string(trig->string1, stringToken.u.string, stringToken.size);
		token stringToken2 = tokenizer->ReadToken();
		get_unquoted_string(trig->string2, stringToken2.u.string, stringToken2.size);
	}
}


/* static */
void
Parser::_ReadObjectBlock(Tokenizer *tokenizer, object_params& obj)
{
	// HEADER GUARD (OB)
	tokenizer->ReadToken();

	obj.ea = tokenizer->ReadToken().u.number;
	if (Core::Get()->Game() == GAME_TORMENT) {
		obj.faction = tokenizer->ReadToken().u.number;
		obj.team = tokenizer->ReadToken().u.number;
	}
	obj.general = tokenizer->ReadToken().u.number;
	obj.race = tokenizer->ReadToken().u.number;
	obj.classs = tokenizer->ReadToken().u.number;
	obj.specific = tokenizer->ReadToken().u.number;
	obj.gender = tokenizer->ReadToken().u.number;
	obj.alignment = tokenizer->ReadToken().u.number;
	for (int32 i = 0; i < 5; i++)
		obj.identifiers[i] = tokenizer->ReadToken().u.number;

	// TODO: Not sure which games supports that
	if (Core::Get()->Game() == GAME_TORMENT) {
		obj.point.x = tokenizer->ReadToken().u.number;
		obj.point.y = tokenizer->ReadToken().u.number;
	}

	token stringToken = tokenizer->ReadToken();
	// HEADER GUARD (OB)
	tokenizer->ReadToken();

	get_unquoted_string(obj.name, stringToken.u.string, stringToken.size);
}


/* static */
void
Parser::_ReadActionBlock(Tokenizer *tokenizer, node* node)
{
	action_params* act = dynamic_cast<action_params*>(node);
	if (act) {
		act->id = tokenizer->ReadToken().u.number;
		act->integer1 = tokenizer->ReadToken().u.number;
		act->where.x = tokenizer->ReadToken().u.number;
		act->where.y = tokenizer->ReadToken().u.number;
		act->integer2 = tokenizer->ReadToken().u.number;
		act->integer3 = tokenizer->ReadToken().u.number;

		// TODO: This removes "" from strings.
		// Should do this from the beginning
		token stringToken = tokenizer->ReadToken();
		get_unquoted_string(act->string1, stringToken.u.string, stringToken.size);
		token stringToken2 = tokenizer->ReadToken();
		get_unquoted_string(act->string2, stringToken2.u.string, stringToken2.size);
	}
}


/* static */
void
Parser::_ReadResponseBlock(Tokenizer *tokenizer, node* node)
{
	response_node* resp = dynamic_cast<response_node*>(node);
	if (resp)
		resp->probability = tokenizer->ReadToken().u.number;
}


/* static */
bool
Parser::_ExtractTriggerName(Tokenizer& tokenizer, ::trigger_params* node)
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
	node->id = GetTriggerID(triggerName);
	if (node->id == -1) {
		std::cerr << Log::Red << "GetTriggerID: no trigger found" << Log::Normal << std::endl;
		return false;
	}
	return true;
}


void
Parser::_ReadNodeHeader(node*& n)
{
	token tok = fTokenizer->ReadToken();
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
	// TODO: Horrible
	static int sActionIndexHACK = 0;

	_ReadNodeHeader(node);
	for (;;) {
		token tok = fTokenizer->ReadToken();
		int blockType =_BlockTypeFromToken(tok);
		if (blockType != -1) {
			// This is a block header tag: could be opening or closing tag
			fTokenizer->RewindToken(tok);
			if (blockType == node->type) {
				// Means the block is open, and this is
				// the closing tag. FixNode will copy the node values
				// to the node specific values.
				// _ReadNodeHeader will do the rest.
				_FixNode(node);
				break;
			} else {
				// Object blocks are no longer nodes
				// so we handle them differently
				if (blockType == BLOCK_OBJECT) {
					if (node->type == BLOCK_TRIGGER) {
						trigger_params* trig = dynamic_cast<trigger_params*>(node);
						_ReadObjectBlock(fTokenizer, trig->object);
					} else if (node->type == BLOCK_ACTION) {
						// TODO: Horrible hack
						action_params* act = dynamic_cast<action_params*>(node);
						object_params* destObjectParams = NULL;
						if (sActionIndexHACK == 0)
							destObjectParams = &act->first;
						else if (sActionIndexHACK == 1)
							destObjectParams = &act->second;
						else if (sActionIndexHACK == 2)
							destObjectParams = &act->third;

						assert(destObjectParams != NULL);

						_ReadObjectBlock(fTokenizer, *destObjectParams);

						if (++sActionIndexHACK > 2)
							sActionIndexHACK = 0;
					}
				} else {
					// We found a nested block:
					// call ourselves recursively
					::node *newNode = NULL;
					_ReadNode(newNode);
					node->AddChild(newNode);
				}
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
	if (node->Value()[0] != '\0')
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
	StringStream stream(node->Value());
	Tokenizer tokenizer(&stream, 0);
	//tokenizer.SetDebug(true);
	switch (node->type) {
		case BLOCK_TRIGGER:
			_ReadTriggerBlock(&tokenizer, node);
			break;
		case BLOCK_ACTION:
			_ReadActionBlock(&tokenizer, node);
			break;
		case BLOCK_RESPONSE:
			_ReadResponseBlock(&tokenizer, node);
			break;
		default:
			// other, no need to read anything
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
	// so we cannot guess the block type
	return -1;
}


// ParameterExtractor
ParameterExtractor::ParameterExtractor(Tokenizer& tokenizer)
	:
	fTokenizer(tokenizer)
{
}


token
ParameterExtractor::_ExtractNextParameter(::trigger_params* node,
								Parameter& parameter)
{
	// TODO: horrible, complex code. Improve, refactor
	std::cout << "ExtractNextParameter" << std::endl;
	token tokenParam = fTokenizer.ReadToken();
	if (tokenParam.type == TOKEN_PARENTHESIS_CLOSED)
		return tokenParam;

	if (parameter.type != Parameter::UNKNOWN)
		parameter.Print();
	if (tokenParam.type == TOKEN_COMMA)
		tokenParam = fTokenizer.ReadToken();

	size_t stringLength = ::strnlen(tokenParam.u.string, sizeof(tokenParam.u.string));
	switch (parameter.type) {
		case Parameter::OBJECT:
		{
			object_params objectNode;
			if (tokenParam.type == TOKEN_QUOTED_STRING)
				get_unquoted_string(objectNode.name, tokenParam.u.string, stringLength);
			else if (tokenParam.type == TOKEN_STRING)
				objectNode.identifiers[0] = IDTable::ObjectID(tokenParam.u.string);
			node->object = objectNode;
			break;
		}
		case Parameter::INTEGER:
			if (parameter.position == 1)
				node->parameter1 = tokenParam.u.number;
			else if (parameter.position == 2)
				node->parameter2 = tokenParam.u.number;
			break;
		case Parameter::INT_ENUM:
		{
			int integerValue;
			IDSResource* idsResource = gResManager->GetIDS(parameter.IDtable.c_str());
			if (idsResource != NULL) {
				integerValue = idsResource->IDForString(tokenParam.u.string);
				gResManager->ReleaseResource(idsResource);
			}
			if (parameter.position == 1)
				node->parameter1 = integerValue;
			else
				node->parameter2 = integerValue;
			break;
		}
		case Parameter::STRING:
		{
			char* destString = NULL;
			if (parameter.position == 1)
				destString = node->string1;
			else if (parameter.position == 2)
				destString = node->string2;
			else
				throw std::runtime_error("wrong parameter position");
			if (tokenParam.type == TOKEN_QUOTED_STRING)
				get_unquoted_string(destString, tokenParam.u.string, stringLength);
			else if (tokenParam.type == TOKEN_STRING) {
				::memcpy(destString, tokenParam.u.string, stringLength);
				destString[stringLength] = '\0';
			}
			break;
		}
		default:
			break;
	}
	return tokenParam;
}



// node
/* static */
node*
node::Create(int type, const char *string)
{
	node* newNode = NULL;
	switch (type) {
		case BLOCK_TRIGGER:
			newNode = new trigger_params;
			break;
		case BLOCK_OBJECT:
			throw std::runtime_error("ERROR BLOCK OBJECT IS NOT HANDLED CORRECTLY!!!!");
			break;
		case BLOCK_ACTION:
			newNode = new action_params;
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
node::Parent() const
{
	return parent;
}


node*
node::Next() const
{
	return next;
}


const char*
node::Value() const
{
	return value;
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
trigger_params::trigger_params()
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
trigger_params::Print() const
{
	if (flags)
		std::cout << "!";

	std::cout << IDTable::TriggerName(id);
	std::cout << "(" << std::dec << (int)id << ", 0x" << std::hex << (int)id << ")";
	std::cout << "(";
	std::cout << std::dec;
	std::cout << parameter1 << ", " << parameter2 << ", " << string1 << ", " << string2 << ")" << std::endl;
	object.Print();
}


object_params*
trigger_params::Object()
{
	return &object;
}


// object
object_params::object_params()
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
object_params::Print() const
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
object_params::Empty() const
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
action_params::action_params()
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
action_params::Print() const
{
	std::cout << IDTable::ActionAt(id);
	std::cout << "(" << std::dec << (int)id << std::hex << ", 0x" << (int)id << ")";
	std::cout << std::dec;
	std::cout << "(";
	std::cout << integer1 << ", ";
	std::cout << "(" << where.x << ", " << where.y << "), ";
	std::cout << integer2 << ", ";
	std::cout << integer3 << ", ";
	std::cout << string1 << ", ";
	std::cout << string2 << ")" << std::endl;
	first.Print();
	second.Print();
	third.Print();
}


object_params*
action_params::First()
{
	return &first;
}


object_params*
action_params::Second()
{
	return &second;
}


object_params*
action_params::Third()
{
	return &third;
}


//response
response_node::response_node()
	:
	probability(100)
{
}


void
response_node::Print() const
{
	std::cout << "probability: " << probability << std::endl;
}
