#include "Parsing.h"

#include "Actions.h"
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
	token _ExtractNextParameter(::action_params* params,
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


/* static */
std::vector<action_params*>
Parser::ActionsFromString(const std::string& string)
{
	std::string localString = string;
	std::vector<action_params*> actionList;
	if (!string.empty()) {
		while (true) {
			action_params* actionParam = ActionFromString(localString);
			if (actionParam != NULL)
				actionList.push_back(actionParam);
			size_t endLine = localString.find('\n');
			if (endLine == localString.length() || endLine == std::string::npos)
				break;
			localString = localString.substr(endLine + 1, string.length());
		}
	}
	return actionList;
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
	} else if (typeString == "P:")
		parameter.type = Parameter::POINT;

	return parameter;
}


static
std::vector<Parameter>
GetFunctionParameters(std::string functionString)
{
	//std::cout << "GetFunctionParameters()" << std::endl;
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
#if 0
	std::cout << "found " << parameters.size() << " parameters." << std::endl;
	std::vector<Parameter>::const_iterator i;
	for (i = parameters.begin(); i != parameters.end(); i++) {
		(*i).Print();
	}
#endif
	return parameters;
}


/* static */
trigger_params*
Parser::TriggerFromString(const std::string& string)
{
	//std::cout << "TriggerFromString()" << std::endl;
	trigger_params* node = new trigger_params();
	StringStream stream(string);
	Tokenizer tokenizer(&stream, 0);
	//tokenizer.SetDebug(true);
	if (!_ExtractTriggerName(tokenizer, node)) {
		//node->Release();
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

	//std::cout << "TriggerFromString() END" << std::endl;
	//node->Print();

	return node;
}


/* static */
action_params*
Parser::ActionFromString(const std::string& string)
{
	//std::cerr << "ActionFromString: " << string << std::endl;
	action_params* params = new action_params();
	StringStream stream(string);
	Tokenizer tokenizer(&stream, 0);
	//tokenizer.SetDebug(true);
	if (!_ExtractActionName(tokenizer, params)) {
		//node->Release();
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
	std::vector<Parameter> paramTypes = GetFunctionParameters(IDTable::ActionName(params->id));
	for (std::vector<Parameter>::const_iterator i = paramTypes.begin();
			i != paramTypes.end(); i++) {
		Parameter parameter = *i;
		extractor._ExtractNextParameter(params, parameter);
	}
	//std::cout << "ActionFromString() END" << std::endl;
	//params->Print();

	return params;
}


std::vector<condition_response*>
Parser::Read()
{
	std::vector<condition_response*> blocks;
	try {
		if (fTokenizer->ReadToken() == token("SC")) {
			condition_response* condResp = NULL;
			while ((condResp = _ReadConditionResponseBlock()) != NULL) {
				blocks.push_back(condResp);
			}
			fTokenizer->ReadToken(); // closing tag SC
		}
	} catch (std::exception& except) {
		std::cerr << Log::Red << "Parser::Read(): " << except.what() << std::endl;
	} catch (...) {
		std::cerr << Log::Red << "Parser::Read(): unknown exception" << std::endl;
	}
	return blocks;
}



void
Parser::Test()
{
	std::string actions[] = {
			"SetGlobal(\"TalkedToMadeen\",\"GLOBAL\",1)",
			"SetGlobalTimer(\"ImoenDream1\",\"GLOBAL\",ONE_DAY)",
			"ClearAllActions()",
			"StartCutSceneMode()",
			"StartCutScene(\"Cut42a\")",
	};

	for (size_t i = 0; i < sizeof(actions) / sizeof(actions[0]); i++) {
		action_params* params = Parser::ActionFromString(actions[i]);
		params->Print();
	}
}


/* static */
void
Parser::_ReadObjectBlock(Tokenizer *tokenizer, object_params& obj)
{
	// HEADER GUARD (OB)
	token h = tokenizer->ReadToken();
	if (h == token("OB")) {
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
		get_unquoted_string(obj.name, stringToken.u.string, stringToken.size);	
		
		// HEADER GUARD (OB)
		token t = tokenizer->ReadToken();
		assert(t == token("OB"));
	} else
		tokenizer->RewindToken(h);
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


/* static */
bool
Parser::_ExtractActionName(Tokenizer& tokenizer, ::action_params* param)
{
	// Action name
	token t = tokenizer.ReadToken();
	if (t.type != TOKEN_STRING)
		return false;

	std::string actionName = t.u.string;
	param->id = GetActionID(actionName);
	if (param->id == -1) {
		std::cerr << Log::Red << "GetActionID: no action found (" << actionName << ")" << Log::Normal << std::endl;
		return false;
	}
	return true;
}


condition_response*
Parser::_ReadConditionResponseBlock()
{
	condition_response* condResp = NULL;
	token h = fTokenizer->ReadToken();
	if (h == token("CR")) {
		//std::cout << "_ReadConditionResponseBlock()" << std::endl;
		condResp = new condition_response;
		_ReadConditionBlock(condResp->conditions);
		_ReadResponseSetBlock(condResp->responseSet);
		
		token t = fTokenizer->ReadToken(); // closing tag
		assert(t == token("CR"));
		//std::cout << "_ReadConditionResponseBlock() end" << std::endl;
	} else
		fTokenizer->RewindToken(h);
	return condResp;
}


void
Parser::_ReadConditionBlock(condition_block& cond)
{
	token h = fTokenizer->ReadToken();
	if (h == token("CO")) {
		//std::cout << "_ReadConditionBlock()" << std::endl;
		trigger_params* trig = NULL;
		int32 i = 0;
		while ((trig = _ReadTriggerBlock()) != NULL) {
			//std::cout << "Condition: found trigger " << i << std::endl;
			cond.triggers.push_back(trig);
			i++;
		}
		token t = fTokenizer->ReadToken(); // closing tag
		assert(t == token("CO"));
		//std::cout << "_ReadConditionBlock() end" << std::endl;
	} else
		fTokenizer->RewindToken(h);
}


trigger_params*
Parser::_ReadTriggerBlock()
{
	trigger_params* trig = NULL;
	token h = fTokenizer->ReadToken();
	if (h == token("TR")) {
		//std::cout << "_ReadTriggerBlock()" << std::endl;
		trig = new trigger_params();

		trig->id = fTokenizer->ReadToken().u.number;
		trig->parameter1 = fTokenizer->ReadToken().u.number;
		trig->flags = fTokenizer->ReadToken().u.number;
		trig->parameter2 = fTokenizer->ReadToken().u.number;
		trig->unknown = fTokenizer->ReadToken().u.number;

		// Strings are quoted. We remove quotes
		token stringToken = fTokenizer->ReadToken();
		get_unquoted_string(trig->string1, stringToken.u.string, stringToken.size);
		token stringToken2 = fTokenizer->ReadToken();
		get_unquoted_string(trig->string2, stringToken2.u.string, stringToken2.size);

		// Object
		_ReadObjectBlock(fTokenizer, *trig->Object());

		token t = fTokenizer->ReadToken(); // closing tag
		assert(t == token("TR"));
		//std::cout << "_ReadTriggerBlock() end" << std::endl;
	} else
		fTokenizer->RewindToken(h);
	return trig;
}


void
Parser::_ReadResponseSetBlock(response_set& respSet)
{
	token h = fTokenizer->ReadToken();
	if (h == token("RS")) {
		//std::cout << "_ReadResponseSetBlock()" << std::endl;
		response_node* resp = NULL;
		while ((resp = _ReadResponseBlock()) != NULL)
			respSet.resp.push_back(resp);
		token t = fTokenizer->ReadToken(); // closing tag
		assert(t == token("RS"));
		//std::cout << "_ReadResponseSetBlock() end" << std::endl;
	} else
		fTokenizer->RewindToken(h);
}


response_node*
Parser::_ReadResponseBlock()
{
	response_node* resp = NULL;
	token h = fTokenizer->ReadToken();
	if (h == token("RE")) {
		//std::cout << "_ReadResponseBlock()" << std::endl;
		resp = new response_node;
		resp->probability = fTokenizer->ReadToken().u.number;
		action_params* act = new action_params();
		while ((act = _ReadActionBlock()) != NULL)
			resp->actions.push_back(act);
		token t = fTokenizer->ReadToken(); // closing tag
		if (!(t == token("RE")))
			return NULL;
		//std::cout << "_ReadResponseBlock() end" << std::endl;
	} else
		fTokenizer->RewindToken(h);
	return resp;
}


action_params*
Parser::_ReadActionBlock()
{
	action_params* act = NULL;
	token h = fTokenizer->ReadToken();
	if (h == token("AC")) {
		//std::cout << "_ReadActionBlock()" << std::endl;
		act = new action_params;
		act->id = fTokenizer->ReadToken().u.number;
		_ReadObjectBlock(fTokenizer, *act->First());
		_ReadObjectBlock(fTokenizer, *act->Second());
		_ReadObjectBlock(fTokenizer, *act->Third());

		act->integer1 = fTokenizer->ReadToken().u.number;
		act->where.x = fTokenizer->ReadToken().u.number;
		act->where.y = fTokenizer->ReadToken().u.number;
		act->integer2 = fTokenizer->ReadToken().u.number;
		act->integer3 = fTokenizer->ReadToken().u.number;

		// TODO: This removes "" from strings.
		// Should do this from the beginning
		token stringToken = fTokenizer->ReadToken();
		get_unquoted_string(act->string1, stringToken.u.string, stringToken.size);
		token stringToken2 = fTokenizer->ReadToken();
		get_unquoted_string(act->string2, stringToken2.u.string, stringToken2.size);

		token t = fTokenizer->ReadToken(); // closing tag
		assert(t == token("AC"));
		//std::cout << "_ReadActionBlock() end" << std::endl;
	} else
		fTokenizer->RewindToken(h);
	return act;
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
	//std::cout << "ExtractNextParameter" << std::endl;
	token tokenParam = fTokenizer.ReadToken();
	if (tokenParam.type == TOKEN_PARENTHESIS_CLOSED)
		return tokenParam;

	//if (parameter.type != Parameter::UNKNOWN)
	//	parameter.Print();
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
			*node->Object() = objectNode;
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


token
ParameterExtractor::_ExtractNextParameter(::action_params* param,
								Parameter& parameter)
{
	// TODO: horrible, complex code. Improve, refactor
	//std::cout << "ExtractNextParameter(ACTION)" << std::endl;
	token tokenParam = fTokenizer.ReadToken();
	if (tokenParam.type == TOKEN_PARENTHESIS_CLOSED)
		return tokenParam;

	//if (parameter.type != Parameter::UNKNOWN)
	//	parameter.Print();
	if (tokenParam.type == TOKEN_COMMA)
		tokenParam = fTokenizer.ReadToken();

	size_t stringLength = ::strnlen(tokenParam.u.string, sizeof(tokenParam.u.string));
	switch (parameter.type) {
		case Parameter::POINT:
			param->where.x = tokenParam.u.number;
			fTokenizer.ReadToken(); // comma
			param->where.y = fTokenizer.ReadToken().u.number;
			break;
		case Parameter::INTEGER:
			if (parameter.position == 1)
				param->integer1 = tokenParam.u.number;
			else if (parameter.position == 2)
				param->integer2 = tokenParam.u.number;
			else if (parameter.position == 3)
				param->integer3 = tokenParam.u.number;
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
				param->integer1 = integerValue;
			else if (parameter.position == 2)
				param->integer2 = integerValue;
			else if (parameter.position == 3)
				param->integer3 = integerValue;
			break;
		}
		case Parameter::STRING:
		{

			char* destString = NULL;
			if (parameter.position == 1)
				destString = param->string1;
			else if (parameter.position == 2)
				destString = param->string2;
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

	//param->Print();

	//std::cout << tokenParam.u.string << std::endl;

	return tokenParam;
}


// trigger_params
trigger_params::trigger_params()
	:
	id(0),
	parameter1(0),
	flags(0),
	parameter2(0),
	unknown(0),
	object(NULL)
{
	string1[0] = '\0';
	string2[0] = '\0';

	object = new object_params();
}


trigger_params::trigger_params(const trigger_params& other)
	:
	id(other.id),
	parameter1(other.id),
	flags(other.flags),
	parameter2(other.parameter2),
	unknown(other.unknown),
	object(NULL)
{
	strcpy(string1, other.string1);
	strcpy(string2, other.string2);
	if (other.object != NULL)
		object = new object_params(*other.object);
	else
		object = new object_params();
}


/* virtual */
trigger_params::~trigger_params()
{
	delete object;
}


trigger_params&
trigger_params::operator=(const trigger_params& other)
{
	id = other.id;
	parameter1 = other.id;
	flags = other.flags;
	parameter2 = other.parameter2;
	unknown = other.unknown;
	object = NULL;
	strcpy(string1, other.string1);
	strcpy(string2, other.string2);
	if (other.object)
		object = new object_params(*other.object);

	return *this;
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
	std::cout << "int1=" << parameter1 << ", ";
	std::cout << "int2=" << parameter2 << ", ";
	std::cout << "string1=" << string1 << ", ";
	std::cout << "string2=" << string2 << ")" << std::endl;
	if (object != NULL)
		object->Print();
}


object_params*
trigger_params::Object() const
{
	return object;
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


object_params::object_params(const object_params& other)
	:
	team(other.team),
	faction(other.faction),
	ea(other.ea),
	general(other.general),
	race(other.race),
	classs(other.classs),
	specific(other.specific),
	gender(other.gender),
	alignment(other.alignment)
{
	memcpy(identifiers, other.identifiers, sizeof(identifiers));
	point = other.point;
	strcpy(name, other.name);
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
	integer3(0),
	fRefCount(1)
{
	where.x = where.y = -1;
	string1[0] = '\0';
	string2[0] = '\0';
}


action_params::action_params(const char* firstParamName, const char* secondParamName)
	:
	id(0),
	integer1(0),
	integer2(0),
	integer3(0),
	fRefCount(1)
{
	where.x = where.y = -1;
	string1[0] = '\0';
	string2[0] = '\0';

	// TODO: Linux does not have strlcpy by default
	::strncpy(First()->name, firstParamName, sizeof(First()->name));
	::strncpy(Second()->name, secondParamName, sizeof(Second()->name));
}


void
action_params::Print() const
{
	std::cout << IDTable::ActionName(id);
	std::cout << "(" << std::dec << (int)id << std::hex << ", 0x" << (int)id << ")";
	std::cout << std::dec;
	std::cout << "(int1=";
	std::cout << integer1 << ", ";
	std::cout << "point=(" << where.x << ", " << where.y << "), ";
	std::cout << "int2=" << integer2 << ", ";
	std::cout << "int3=" << integer3 << ", ";
	std::cout << "string1=" << string1 << ", ";
	std::cout << "string2=" << string2 << ")" << std::endl;
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


void
action_params::Acquire()
{
	fRefCount++;
}


void
action_params::Release()
{
	if (--fRefCount == 0)
		delete this;
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
