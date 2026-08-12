#ifndef __PARSING_H
#define __PARSING_H

#include "IETypes.h"
#include "ScriptObjects.h"
#include "SupportDefs.h"
#include "Tokenizer.h"

#include <vector>



class Parameter;
class Parser {
public:
	Parser();
	Parser(const Parser&) = delete;
	~Parser();

	void SetTo(Stream *stream);
	std::vector<condition_response*> Read();
	void SetDebug(bool debug);

	static std::vector<trigger_params*> TriggersFromString(const std::string& string);
	static trigger_params* TriggerFromString(const std::string& string);

	static std::vector<action_params*> ActionsFromString(const std::string& string);
	static action_params* ActionFromString(const std::string& string);

	static void Test();

	Parser& operator=(const Parser&) = delete;

private:
	bool _IsNext(const char* tag);
	void _Expect(const char* tag);

	condition_response* _ReadConditionResponseBlock();
	void _ReadConditionBlock(condition_block& param);
	trigger_params* _ReadTriggerBlock();

	void _ReadResponseSetBlock(response_set& respSet);
	response_node* _ReadResponseBlock();
	action_params* _ReadActionBlock();

	void _ReadObjectBlock(object_params& obj);

	static bool _ExtractTriggerName(Tokenizer& tokenizer, ::trigger_params* triggerNode);
	static bool _ExtractActionName(Tokenizer& tokenizer, ::action_params* param);

	Stream *fStream;
	Tokenizer *fTokenizer;
	bool fDebug;
};

#endif // __PARSING_H
