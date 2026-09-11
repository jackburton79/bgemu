#include "ParameterHandlers.h"

#include "IDSResource.h"
#include "ResManager.h"
#include "Utils.h"

#include <cstring>


// ObjectParameterHandler
ObjectParameterHandler::ObjectParameterHandler()
{
}


token
ObjectParameterHandler::ExtractForTrigger(const token& param, trigger_params* node, int position)
{
	_AssignObjectToTrigger(param, node);
	return param;
}


token
ObjectParameterHandler::ExtractForAction(const token& param, action_params* node, int position)
{
	_AssignObjectToAction(param, node, position);
	return param;
}


void
ObjectParameterHandler::_AssignObjectToTrigger(const token& param, trigger_params* node)
{
	object_params objectNode;
	size_t stringLength = ::strnlen(param.u.string, sizeof(param.u.string));
	
	if (param.type == TOKEN_QUOTED_STRING)
		get_unquoted_string(objectNode.name, param.u.string, stringLength);
	else if (param.type == TOKEN_STRING)
		objectNode.identifiers[0] = IDTable::ObjectID(param.u.string);
	
	*node->Object() = objectNode;
}


void
ObjectParameterHandler::_AssignObjectToAction(const token& param, action_params* node, int position)
{
	object_params objectNode;
	size_t stringLength = ::strnlen(param.u.string, sizeof(param.u.string));
	
	if (param.type == TOKEN_QUOTED_STRING)
		get_unquoted_string(objectNode.name, param.u.string, stringLength);
	else if (param.type == TOKEN_STRING)
		objectNode.identifiers[0] = IDTable::ObjectID(param.u.string);
	
	if (position == 1)
		*node->First() = objectNode;
	else if (position == 2)
		*node->Second() = objectNode;
	else if (position == 3)
		*node->Third() = objectNode;
}


// StringParameterHandler
StringParameterHandler::StringParameterHandler()
{
}


token
StringParameterHandler::ExtractForTrigger(const token& param, trigger_params* node, int position)
{
	_AssignStringToTrigger(param, node, position);
	return param;
}


token
StringParameterHandler::ExtractForAction(const token& param, action_params* node, int position)
{
	_AssignStringToAction(param, node, position);
	return param;
}


void
StringParameterHandler::_AssignStringToTrigger(const token& param, trigger_params* node, int position)
{
	char* destString = NULL;
	if (position == 1)
		destString = node->string1;
	else if (position == 2)
		destString = node->string2;
	else
		throw std::runtime_error("wrong parameter position");
	
	size_t stringLength = ::strnlen(param.u.string, sizeof(param.u.string));
	if (param.type == TOKEN_QUOTED_STRING)
		get_unquoted_string(destString, param.u.string, stringLength);
	else if (param.type == TOKEN_STRING) {
		::memcpy(destString, param.u.string, stringLength);
		destString[stringLength] = '\0';
	}
}


void
StringParameterHandler::_AssignStringToAction(const token& param, action_params* node, int position)
{
	char* destString = NULL;
	if (position == 1)
		destString = node->string1;
	else if (position == 2)
		destString = node->string2;
	else
		throw std::runtime_error("wrong parameter position");
	
	size_t stringLength = ::strnlen(param.u.string, sizeof(param.u.string));
	if (param.type == TOKEN_QUOTED_STRING)
		get_unquoted_string(destString, param.u.string, stringLength);
	else if (param.type == TOKEN_STRING) {
		::memcpy(destString, param.u.string, stringLength);
		destString[stringLength] = '\0';
	}
}


// IntegerParameterHandler
IntegerParameterHandler::IntegerParameterHandler()
{
}


token
IntegerParameterHandler::ExtractForTrigger(const token& param, trigger_params* node, int position)
{
	_AssignIntegerToTrigger(param, node, position);
	return param;
}


token
IntegerParameterHandler::ExtractForAction(const token& param, action_params* node, int position)
{
	_AssignIntegerToAction(param, node, position);
	return param;
}


void
IntegerParameterHandler::_AssignIntegerToTrigger(const token& param, trigger_params* node, int position)
{
	if (position == 1)
		node->parameter1 = param.u.number;
	else if (position == 2)
		node->parameter2 = param.u.number;
}


void
IntegerParameterHandler::_AssignIntegerToAction(const token& param, action_params* node, int position)
{
	if (position == 1)
		node->integer1 = param.u.number;
	else if (position == 2)
		node->integer2 = param.u.number;
	else if (position == 3)
		node->integer3 = param.u.number;
}


// IntEnumParameterHandler
IntEnumParameterHandler::IntEnumParameterHandler(const std::string& idTableName)
	: fIDTableName(idTableName)
{
}


token
IntEnumParameterHandler::ExtractForTrigger(const token& param, trigger_params* node, int position)
{
	int value = _GetEnumValue(fIDTableName.c_str(), param.u.string);
	_AssignEnumToTrigger(value, node, position);
	return param;
}


token
IntEnumParameterHandler::ExtractForAction(const token& param, action_params* node, int position)
{
	int value = _GetEnumValue(fIDTableName.c_str(), param.u.string);
	_AssignEnumToAction(value, node, position);
	return param;
}


int
IntEnumParameterHandler::_GetEnumValue(const char* idsName, const char* tokenString) const
{
	int value = 0;
	IDSResource* ids = gResManager->GetIDS(idsName);
	if (ids != NULL) {
		value = ids->IDForString(tokenString);
		gResManager->ReleaseResource(ids);
	}
	return value;
}


void
IntEnumParameterHandler::_AssignEnumToTrigger(int value, trigger_params* node, int position)
{
	if (position == 1)
		node->parameter1 = value;
	else if (position == 2)
		node->parameter2 = value;
}


void
IntEnumParameterHandler::_AssignEnumToAction(int value, action_params* node, int position)
{
	if (position == 1)
		node->integer1 = value;
	else if (position == 2)
		node->integer2 = value;
	else if (position == 3)
		node->integer3 = value;
}


// PointParameterHandler
PointParameterHandler::PointParameterHandler(Tokenizer& tokenizer)
	: fTokenizer(tokenizer)
{
}


token
PointParameterHandler::ExtractForTrigger(const token& param, trigger_params* node, int position)
{
	// POINT parameters are not used in triggers, but keep interface consistent
	return param;
}


token
PointParameterHandler::ExtractForAction(const token& param, action_params* node, int position)
{
	int x = param.u.number;
	fTokenizer.ReadToken(); // consume comma
	token yToken = fTokenizer.ReadToken();
	int y = yToken.u.number;
	
	_AssignPointToAction(x, y, node);
	return param;
}


void
PointParameterHandler::_AssignPointToAction(int x, int y, action_params* node)
{
	node->where.x = x;
	node->where.y = y;
}
