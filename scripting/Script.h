#pragma once

#include <vector>

#include "ScriptObjects.h"


enum block_type {
	BLOCK_SCRIPT,
	BLOCK_CONDITION_RESPONSE,
	BLOCK_CONDITION,
	BLOCK_TRIGGER,
	BLOCK_ACTION,
	BLOCK_OBJECT,
	BLOCK_RESPONSESET,
	BLOCK_RESPONSE,
	BLOCK_UNKNOWN
};

class Actor;
class Object;
class Script {
public:
	Script(std::vector<condition_response*> root);
	~Script();

	static void SetDebug(bool debug);

	void Print() const;

	// convenience calls
	static Object*			GetTriggerObject(const Object* object, trigger_params* start);
	static Object*			GetSenderObject(const Object* object, action_params* start);
	static Object*			GetTargetObject(const Object* object, action_params* start);

	void Execute(bool &continuing, bool& action);

	// Advances the cutscene script by (at most) one condition_response
	// block per call: the caller is expected to call this once per round
	// until it returns true. Unlike Execute() (which rescans the whole
	// vector from the start every round, for regular AI priority logic),
	// a cutscene is a strictly ordered sequence of beats: each block runs
	// at most once, in order, and a block whose condition is not yet true
	// blocks progress (it's retried next call, not skipped) - this is how
	// a beat can wait on e.g. ActionListEmpty(...) before letting the next
	// beat start.
	// Returns true once every block has been processed.
	bool ExecuteCutscene();

	Object* Sender();
	void SetSender(Object* object);

	static bool EvaluateTrigger(Object* sender, trigger_params* trig, int& orTrig);

	static Object* ResolveIdentifier(const Object* object, object_params* node, const int id);
	static Object* GetObject(const Object* source, object_params* node);

private:
	bool _EvaluateConditionBlock(condition_block& block);

	bool _HandleResponseSet(response_set& responseSet);
	bool _HandleAction(action_params* act);

	static Actor* _GetIdentifiers(const Object* source, object_params* node,
					std::vector<std::string>& identifiersList);

	std::vector<condition_response*> fConditionResponses;
	Object* fSender;

	// Index of the next condition_response block ExecuteCutscene() will
	// process. Only ever moves forward.
	size_t fCutsceneIndex;

	// Set by _HandleAction() when it processes an ACTIONOVERRIDE(O:Object*)
	// action; consumed (and cleared) by the very next action in the same
	// list, which runs against this object instead of the script's normal
	// sender.
	Object* fPendingOverrideTarget;

	static bool sDebug;
};

