#include "ScriptObjects.h"

#include "Core.h"
#include "Log.h"
#include "ResManager.h"

#include <cstring>
#include <iostream>

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
	parameter1(other.parameter1),
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
	if (&other == this)
		return *this;

	id = other.id;
	parameter1 = other.parameter1;
	flags = other.flags;
	parameter2 = other.parameter2;
	unknown = other.unknown;
	delete object;
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
	if (Core::Get()->Game() == game::GAME_TORMENT) {
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
	if (Core::Get()->Game() == game::GAME_TORMENT)
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
	::strncpy(First()->name, firstParamName, sizeof(First()->name) - 1);
	::strncpy(Second()->name, secondParamName, sizeof(Second()->name) - 1);
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
