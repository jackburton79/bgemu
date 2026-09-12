#include "Parsing.h"

#include <algorithm>
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
	bool fDone;

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


// Parses one "X:Name*IDS" signature token (e.g. "O:Target*", "I:Time*",
// "I:ScrollSpeed*Scroll") from an ACTION.IDS/TRIGGER.IDS-style function
// signature. Malformed input (no ':' or no '*', or one immediately after
// the other) yields a Parameter::UNKNOWN instead of guessing from
// clamped-but-wrong substring bounds.
static
Parameter
ParameterFromString(const std::string& string, int& stringPos, int& integerPos)
{
	Parameter parameter;
	if (string.size() < 2)
		return parameter;

	size_t colonPos = string.find(':');
	size_t starPos = string.find('*');
	if (colonPos == std::string::npos || starPos == std::string::npos
			|| starPos <= colonPos + 1) {
		return parameter;
	}

	parameter.name = string.substr(colonPos + 1, starPos - colonPos - 1);
	std::string valueIDS = string.substr(starPos + 1);

	std::string typeString = string.substr(0, 2);
	if (typeString == "O:") {
		parameter.type = Parameter::OBJECT;
	} else if (typeString == "S:") {
		parameter.type = Parameter::STRING;
		parameter.position = stringPos++;
	} else if (typeString == "I:") {
		if (valueIDS.empty())
			parameter.type = Parameter::INTEGER;
		else {
			parameter.type = Parameter::INT_ENUM;
			parameter.IDtable = valueIDS;
		}
		parameter.position = integerPos++;
	} else if (typeString == "P:")
		parameter.type = Parameter::POINT;

	return parameter;
}


// Parses an ACTION.IDS/TRIGGER.IDS function signature (e.g.
// "MoveViewObject(O:Target*,I:ScrollSpeed*Scroll)") into its ordered
// Parameter list, with S:/I: positions numbered separately and O:
// positions assigned per the action_params::First()/Second()/Third()
// convention Script::GetSenderObject()/GetTargetObject() read: a single
// O: parameter is always the target, landing in Second(); with more than
// one, they fill First(), Second(), Third() in declaration order.
static
std::vector<Parameter>
GetFunctionParameters(const std::string& functionString)
{
	StringStream stream(functionString);
	Tokenizer tokenizer(&stream, 0);

	std::vector<Parameter> parameters;
	token functionName = tokenizer.ReadToken();
	token parens = tokenizer.ReadToken();
	if (functionName.type != TOKEN_STRING
			|| parens.type != TOKEN_PARENTHESIS_OPEN)
		return parameters;

	int stringPos = 1;
	int integerPos = 1;
	for (;;) {
		token t = tokenizer.ReadToken();
		if (t.type == TOKEN_PARENTHESIS_CLOSED)
			break;
		if (t.type == TOKEN_COMMA)
			continue;
		parameters.push_back(ParameterFromString(t.u.string, stringPos, integerPos));
	}

	std::vector<size_t> objectIndices;
	for (size_t i = 0; i < parameters.size(); i++) {
		if (parameters[i].type == Parameter::OBJECT)
			objectIndices.push_back(i);
	}
	if (objectIndices.size() == 1)
		parameters[objectIndices[0]].position = 2;
	else {
		for (size_t i = 0; i < objectIndices.size(); i++)
			parameters[objectIndices[i]].position = i + 1;
	}
	return parameters;
}


// Real compiled .bcs data packs Global/GlobalGT/GlobalLT/SetGlobal/
// IncrementGlobal's two string parameters ("Name" and "Area") into a
// single string field - Area first, always exactly 6 characters, then
// the bare name (see Variables::GetNameAndScope()'s own comment) -
// instead of the independent string1/string2 fields every other
// two-S:-parameter trigger/action uses. The generic per-parameter
// extraction above has no way to special-case this: it stores Name into
// string1 and Area into string2 like anything else. Repack them here
// into the same combined representation compiled data already carries,
// so a trigger/action parsed from text (DLG state triggers, DLG-embedded
// action text, Evaluate-Trigger/Queue-Action) ends up identical to one
// read from a real .bcs file, instead of Variables::GetScoped()/
// SetScoped() misreading an unpacked Name as "<6-char bogus scope><rest
// of the name>".
static void
_PackVariableScopeStrings(char* name, char* area)
{
	std::string combined(area, strnlen(area, 6));
	combined.resize(6, ' ');
	combined += name;
	strncpy(name, combined.c_str(), 47);
	name[47] = '\0';
	area[0] = '\0';
}


static bool
_TriggerPacksVariableScope(int id)
{
	return id == 0x400F  // Global(S:Name*,S:Area*,I:Value*)
		|| id == 0x4034  // GlobalGT
		|| id == 0x4035; // GlobalLT
}


static bool
_ActionPacksVariableScope(int id)
{
	return id == 30    // SETGLOBAL(S:NAME*,S:AREA*,I:VALUE*)
		|| id == 109;  // INCREMENTGLOBAL
}


/* static */
trigger_params*
Parser::TriggerFromString(const std::string& string)
{
	trigger_params* node = new trigger_params();
	StringStream stream(string);
	Tokenizer tokenizer(&stream, 0);
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
		delete node;
		return NULL;
	}
	ParameterExtractor extractor(tokenizer);
	std::vector<Parameter> paramTypes = GetFunctionParameters(IDTable::TriggerName(node->id));
	for (auto parameter: paramTypes) {
		extractor._ExtractNextParameter(node, parameter);
	}

	if (_TriggerPacksVariableScope(node->id))
		_PackVariableScopeStrings(node->string1, node->string2);

	return node;
}


/* static */
action_params*
Parser::ActionFromString(const std::string& string)
{
	action_params* params = new action_params();
	StringStream stream(string);
	Tokenizer tokenizer(&stream, 0);
	if (!_ExtractActionName(tokenizer, params)) {
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
		return nullptr;
	}

	if (_ActionPacksVariableScope(params->id))
		_PackVariableScopeStrings(params->string1, params->string2);

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
	if (Core::Get()->Game() == game::GAME_TORMENT) {
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
	if (Core::Get()->Game() == game::GAME_TORMENT) {
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


// Byte size of every fixed C buffer a parsed token can land in
// (object_params::name, action_params/trigger_params::string1/2 - see
// ScriptObjects.h). tokenParam.size can reach this exact value when a
// token fills its own same-sized buffer with no room left for a NUL
// (Tokenizer.cpp's strnlen()-based size computation doesn't reserve one) -
// _CopyBoundedString() below clamps to kFieldSize-1 so the destination is
// always left NUL-terminated instead of copying past it.
static const size_t kFieldSize = 48;


// Copies a (possibly quoted) string token into a fixed kFieldSize C
// buffer, truncating rather than overflowing if the token doesn't fit.
static void
_CopyBoundedString(char* dest, token& tokenParam)
{
	size_t copyLength = std::min((size_t)tokenParam.size, kFieldSize - 1);
	if (tokenParam.type == TOKEN_QUOTED_STRING)
		get_unquoted_string(dest, tokenParam.u.string, copyLength);
	else if (tokenParam.type == TOKEN_STRING) {
		::memcpy(dest, tokenParam.u.string, copyLength);
		dest[copyLength] = '\0';
	}
}


// Fills `dest` from a single already-read O: token - shared by both
// action_params and trigger_params extraction below, which each resolve
// which of their own object_params slots `dest` points to (trigger_params
// has only node->Object(); action_params has First()/Second()/Third(),
// picked by parameter.position) before calling this.
static void
_FillObjectParameter(object_params* dest, token& tokenParam)
{
	if (tokenParam.type == TOKEN_QUOTED_STRING)
		_CopyBoundedString(dest->name, tokenParam);
	else if (tokenParam.type == TOKEN_STRING)
		dest->identifiers[0] = IDTable::ObjectID(tokenParam.u.string);
}


// Resolves which of a trigger's/action's two S: string fields
// `parameter.position` (1-based) refers to - both structs expose
// string1/string2 as identically-typed public fields, so one template
// covers both without a shared base class.
template<typename Params>
static char*
_StringFieldAt(Params* params, int position)
{
	if (position == 1)
		return params->string1;
	if (position == 2)
		return params->string2;
	throw std::runtime_error("wrong parameter position");
}


// ParameterExtractor
ParameterExtractor::ParameterExtractor(Tokenizer& tokenizer)
	:
	fTokenizer(tokenizer),
	fDone(false)
{
}


token
ParameterExtractor::_ExtractNextParameter(::trigger_params* node,
								Parameter& parameter)
{
	// The TEXT form is free to omit trailing parameters (e.g.
	// HasItem("DAGG01") against TRIGGER.IDS' HasItem(S:RESREF*,O:OBJECT*)):
	// once the closing parenthesis has been seen, leave every remaining
	// declared parameter at its default rather than reading past it.
	if (fDone)
		return token();

	token tokenParam = _ReadParameterToken();
	if (fDone)
		return tokenParam;

	switch (parameter.type) {
		case Parameter::OBJECT:
			_FillObjectParameter(node->Object(), tokenParam);
			break;
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
			_CopyBoundedString(_StringFieldAt(node, parameter.position), tokenParam);
			break;
		default:
			break;
	}
	return tokenParam;
}


token
ParameterExtractor::_ExtractNextParameter(::action_params* param,
								Parameter& parameter)
{
	if (fDone)
		return token();

	token tokenParam = _ReadParameterToken();
	if (fDone)
		return tokenParam;

	switch (parameter.type) {
		case Parameter::POINT:
			param->where.x = tokenParam.u.number;
			fTokenizer.ReadToken(); // comma
			param->where.y = fTokenizer.ReadToken().u.number;
			break;
		case Parameter::OBJECT:
			if (parameter.position == 1)
				_FillObjectParameter(param->First(), tokenParam);
			else if (parameter.position == 2)
				_FillObjectParameter(param->Second(), tokenParam);
			else if (parameter.position == 3)
				_FillObjectParameter(param->Third(), tokenParam);
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
			_CopyBoundedString(_StringFieldAt(param, parameter.position), tokenParam);
			break;
		default:
			break;
	}

	return tokenParam;
}


token
ParameterExtractor::_ReadParameterToken()
{
	token t = fTokenizer.ReadToken();
	if (t.type == TOKEN_PARENTHESIS_CLOSED) {
		// Put it back so whatever reads the call's closing parenthesis next
		// (TriggerFromString()/ActionFromString()) still sees it.
		fTokenizer.RewindToken(t);
		fDone = true;
		return t;
	}

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
