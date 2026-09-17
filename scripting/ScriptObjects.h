#pragma once

#include "IETypes.h"
#include "SupportDefs.h"


struct object_params {
	object_params();
	object_params(const object_params& other);

	void Print() const;
	bool Empty() const;

	int team;
	int faction;
	int ea;
	int general;
	int race;
	int classs;
	int specific;
	int gender;
	int alignment;
	int identifiers[5];
	IE::point point;
	char name[48];

	// Exact object instance to target, bypassing name-based lookup -
	// set only for actions we construct ourselves from a player click
	// (see Actor::ClickedOn()), never by real parsed BCS/DLG data (which
	// only ever carries a name/identifier, per the on-disk format).
	// AreaRoom::GetObject(const char*) resolves "name" to whichever
	// actor with that resref comes first in its actor list - correct
	// for script-authored references (real content doesn't duplicate a
	// name it cares about resolving unambiguously), but wrong for a
	// click: two actors sharing a resref made the click always resolve
	// to one specific one of them, regardless of which was actually
	// under the cursor. kInvalidGlobalID means "not set", falling back
	// to the name/identifier lookup exactly as before.
	uint16 globalId;
};

const uint16 kInvalidGlobalID = (uint16)-1;


struct trigger_params {
	void Print() const;
	object_params* Object() const;

	int id;
	int parameter1;
	int flags;
	int parameter2;
	int unknown;
	char string1[48];
	char string2[48];

	trigger_params();
	trigger_params(const trigger_params& other);
	~trigger_params();

	trigger_params& operator=(const trigger_params& other);

private:
	object_params* object = NULL;
};


struct action_params {
	action_params();
	action_params(const char* firstParamName, const char* secondParamName);

	void Print() const;
	object_params* First();
	object_params* Second();
	object_params* Third();

	void Acquire();
	void Release();

	int id;
	int integer1;
	IE::point where;
	int integer2;
	int integer3;
	char string1[48];
	char string2[48];

private:
	object_params first;
	object_params second;
	object_params third;

	int32 fRefCount;
};


struct response_node {
	response_node();
	~response_node();

	response_node(const response_node&) = delete;
	response_node& operator=(const response_node&) = delete;

	void Print() const;
	int probability;
	std::vector<action_params*> actions;
};


struct response_set {
	response_set() = default;
	~response_set();

	response_set(const response_set&) = delete;
	response_set& operator=(const response_set&) = delete;

	std::vector<response_node*> resp;
};


struct condition_block {
	condition_block() = default;
	~condition_block();

	condition_block(const condition_block&) = delete;
	condition_block& operator=(const condition_block&) = delete;

	std::vector<trigger_params*> triggers;
};

struct condition_response {
	condition_response() = default;
	~condition_response() = default;

	condition_response(const condition_response&) = delete;
	condition_response& operator=(const condition_response&) = delete;

	condition_block conditions;
	response_set responseSet;
};
