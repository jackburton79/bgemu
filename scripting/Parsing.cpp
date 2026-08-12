#include "Parsing.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>

#include "Actions.h"
#include "Core.h"
#include "IDSResource.h"
#include "Log.h"
#include "ResManager.h"
#include "StringStream.h"
#include "Triggers.h"
#include "Utils.h"


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



class ParameterExtractor {
public:
	ParameterExtractor(Tokenizer& tokenizer);
	token _ExtractNextParameter(::trigger_params* triggerNode,
								Parameter& parameter);
	token _ExtractNextParameter(::action_params* params,
									Parameter& parameter);
private:
	Tokenizer& fTokenizer;

	token _ReadParameterToken();
	int _EnumValue(const char* idsName, const char* string);
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
GetFunctionParameters(const std::string& functionString)
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
	for (auto param: parameters) {
		param.Print();
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
		delete node;
		//node->Release();
		return NULL;
	}

	// Opening parenthesis
	try {
		token parenthesis = tokenizer.ReadToken();
		assert(parenthesis.type == TOKEN_PARENTHESIS_OPEN);
	} catch (std::exception& e)	{
		std::cerr << Log::Yellow << e.what() << Log::Normal << std::endl;
		delete node;
		return NULL;
	}
	ParameterExtractor extractor(tokenizer);
	std::vector<Parameter> paramTypes = GetFunctionParameters(IDTable::TriggerName(node->id));
	for (auto parameter: paramTypes) {
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
		delete params;
		return NULL;
	}

	// Opening parenthesis
	try {
		token parenthesis = tokenizer.ReadToken();
		assert(parenthesis.type == TOKEN_PARENTHESIS_OPEN);
	} catch (std::exception& e)	{
		std::cerr << Log::Yellow << e.what() << Log::Normal << std::endl;
		delete params;
		return NULL;
	}
	// TODO: This isn't too reliable: there are cases where an action has two forms
	// with the same id: one with some parameters and one with other or no parameters
	ParameterExtractor extractor(tokenizer);
	std::vector<Parameter> paramTypes = GetFunctionParameters(IDTable::ActionName(params->id));
	try {
		for (auto parameter: paramTypes) {
			extractor._ExtractNextParameter(params, parameter);
		}
	} catch (const std::exception& exception) {
		std::cerr << "Parser::ActionFromString(): got exception " << exception.what() << std::endl;
		delete params;
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


bool
Parser::_IsNext(const char* tag)
{
	token t = fTokenizer->ReadToken();
	fTokenizer->RewindToken(t);
	return t == token(tag);
}


void
Parser::_Expect(const char* tag)
{
	token t = fTokenizer->ReadToken();
	if (!(t == token(tag)))
		throw std::runtime_error(std::string("Expected token ") + tag);
}


/* static */
void
Parser::_ReadObjectBlock(object_params& obj)
{
	// HEADER GUARD (OB)
	if (!_IsNext("OB"))
		return;

	_Expect("OB");

	obj.ea = fTokenizer->ReadToken().u.number;
	if (Core::Get()->Game() == GAME_TORMENT) {
		obj.faction = fTokenizer->ReadToken().u.number;
		obj.team = fTokenizer->ReadToken().u.number;
	}
	obj.general = fTokenizer->ReadToken().u.number;
	obj.race = fTokenizer->ReadToken().u.number;
	obj.classs = fTokenizer->ReadToken().u.number;
	obj.specific = fTokenizer->ReadToken().u.number;
	obj.gender = fTokenizer->ReadToken().u.number;
	obj.alignment = fTokenizer->ReadToken().u.number;
	for (int32 i = 0; i < 5; i++)
		obj.identifiers[i] = fTokenizer->ReadToken().u.number;

	// TODO: Not sure which games supports that
	if (Core::Get()->Game() == GAME_TORMENT) {
		obj.point.x = fTokenizer->ReadToken().u.number;
		obj.point.y = fTokenizer->ReadToken().u.number;
	}
	token stringToken = fTokenizer->ReadToken();
	get_unquoted_string(obj.name, stringToken.u.string, stringToken.size);

	// HEADER GUARD (OB)
	_Expect("OB");
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
	if (!_IsNext("CR"))
		return NULL;

	_Expect("CR");

	condition_response* condResp = new condition_response;
	_ReadConditionBlock(condResp->conditions);
	_ReadResponseSetBlock(condResp->responseSet);

	_Expect("CR");

	return condResp;
}


void
Parser::_ReadConditionBlock(condition_block& cond)
{
	if (!_IsNext("CO"))
		return;

	_Expect("CO");

	trigger_params* trig = NULL;
	while ((trig = _ReadTriggerBlock()) != NULL) {
		cond.triggers.push_back(trig);
	}

	_Expect("CO");

}


trigger_params*
Parser::_ReadTriggerBlock()
{
	if (!_IsNext("TR"))
		return NULL;

	_Expect("TR");

	trigger_params* trig = new trigger_params();
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
	_ReadObjectBlock(*trig->Object());

	_Expect("TR");

	return trig;
}


void
Parser::_ReadResponseSetBlock(response_set& respSet)
{
	if (!_IsNext("RS"))
		return;

	_Expect("RS");

	response_node* resp = NULL;
	while ((resp = _ReadResponseBlock()) != NULL)
		respSet.resp.push_back(resp);

	_Expect("RS");
}


response_node*
Parser::_ReadResponseBlock()
{
	if (!_IsNext("RE"))
		return NULL;

	_Expect("RE");

	response_node* resp = new response_node;
	resp->probability = fTokenizer->ReadToken().u.number;

	action_params* act = NULL;
	while ((act = _ReadActionBlock()) != NULL)
		resp->actions.push_back(act);

	_Expect("RE");

	return resp;
}


action_params*
Parser::_ReadActionBlock()
{
	if (!_IsNext("AC"))
		return NULL;

	_Expect("AC");

	action_params* act = new action_params;
	act->id = fTokenizer->ReadToken().u.number;
	_ReadObjectBlock(*act->First());
	_ReadObjectBlock(*act->Second());
	_ReadObjectBlock(*act->Third());

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

	_Expect("AC");

	return act;
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
	token tokenParam = _ReadParameterToken();

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
			int integerValue = _EnumValue(parameter.IDtable.c_str(), tokenParam.u.string);
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
	token tokenParam = _ReadParameterToken();

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
			int integerValue = _EnumValue(parameter.IDtable.c_str(), tokenParam.u.string);
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


token
ParameterExtractor::_ReadParameterToken()
{
	token t = fTokenizer.ReadToken();
	if (t.type == TOKEN_PARENTHESIS_CLOSED)
		throw std::runtime_error("Expecting parameter, got closing parenthesis");

	if (t.type == TOKEN_COMMA)
		t = fTokenizer.ReadToken();

	return t;
}


int
ParameterExtractor::_EnumValue(const char* idsName, const char* tokenString)
{
	int value = 0;
	IDSResource* ids = gResManager->GetIDS(idsName);
	if (ids != NULL) {
		value = ids->IDForString(tokenString);
		gResManager->ReleaseResource(ids);
	}

	return value;
}
