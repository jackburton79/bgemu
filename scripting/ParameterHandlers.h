#pragma once

#include <string>
#include "ScriptObjects.h"
#include "Tokenizer.h"


class ParameterHandler {
public:
	virtual ~ParameterHandler() = default;
	
	// Extract and assign parameter to trigger
	virtual token ExtractForTrigger(const token& param, trigger_params* node, int position) = 0;
	
	// Extract and assign parameter to action
	virtual token ExtractForAction(const token& param, action_params* node, int position) = 0;
};


// Handler for OBJECT parameters
class ObjectParameterHandler : public ParameterHandler {
public:
	ObjectParameterHandler();
	
	token ExtractForTrigger(const token& param, trigger_params* node, int position) override;
	token ExtractForAction(const token& param, action_params* node, int position) override;

private:
	void _AssignObjectToTrigger(const token& param, trigger_params* node);
	void _AssignObjectToAction(const token& param, action_params* node, int position);
};


// Handler for STRING parameters
class StringParameterHandler : public ParameterHandler {
public:
	StringParameterHandler();
	
	token ExtractForTrigger(const token& param, trigger_params* node, int position) override;
	token ExtractForAction(const token& param, action_params* node, int position) override;

private:
	void _AssignStringToTrigger(const token& param, trigger_params* node, int position);
	void _AssignStringToAction(const token& param, action_params* node, int position);
};


// Handler for INTEGER parameters
class IntegerParameterHandler : public ParameterHandler {
public:
	IntegerParameterHandler();
	
	token ExtractForTrigger(const token& param, trigger_params* node, int position) override;
	token ExtractForAction(const token& param, action_params* node, int position) override;

private:
	void _AssignIntegerToTrigger(const token& param, trigger_params* node, int position);
	void _AssignIntegerToAction(const token& param, action_params* node, int position);
};


// Handler for INT_ENUM parameters (requires IDTable lookup)
class IntEnumParameterHandler : public ParameterHandler {
public:
	explicit IntEnumParameterHandler(const std::string& idTableName);
	
	token ExtractForTrigger(const token& param, trigger_params* node, int position) override;
	token ExtractForAction(const token& param, action_params* node, int position) override;

private:
	std::string fIDTableName;
	int _GetEnumValue(const char* idsName, const char* tokenString) const;
	void _AssignEnumToTrigger(int value, trigger_params* node, int position);
	void _AssignEnumToAction(int value, action_params* node, int position);
};


// Handler for POINT parameters
class PointParameterHandler : public ParameterHandler {
public:
	PointParameterHandler(Tokenizer& tokenizer);
	
	token ExtractForTrigger(const token& param, trigger_params* node, int position) override;
	token ExtractForAction(const token& param, action_params* node, int position) override;

private:
	Tokenizer& fTokenizer;
	void _AssignPointToAction(int x, int y, action_params* node);
};
