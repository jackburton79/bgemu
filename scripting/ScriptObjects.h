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
};


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
	void Print() const;
	int probability;
	std::vector<action_params*> actions;
};


struct response_set {
	std::vector<response_node*> resp;
};


struct condition_block {
	std::vector<trigger_params*> triggers;
};

struct condition_response {
	condition_block conditions;
	response_set responseSet;
};
