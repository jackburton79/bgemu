#include "Action.h"
#include "Actor.h"
#include "AreaResource.h"
#include "Core.h"
#include "CreResource.h"
#include "Door.h"
#include "Game.h"
#include "GUI.h"
#include "IDSResource.h"
#include "Parsing.h"
#include "Party.h"
#include "Region.h"
#include "ResManager.h"
#include "RoomBase.h"
#include "RoundResults.h"
#include "Script.h"
#include "Timer.h"

#include <algorithm>
#include <assert.h>
#include <sstream>


bool Script::sDebug = false;

static int sIndent = 0;
static void IndentMore()
{
	sIndent++;
}


static void IndentLess()
{
	if (--sIndent < 0)
		sIndent = 0;
}


static void PrintIndentation()
{
	for (int i = 0; i < sIndent; i++)
		printf(" ");
}


static void
VariableGetScopeName(const char* variable, std::string& varScope, std::string& varName)
{
	std::string variableScope;
	varScope.append(variable, 6);
	std::string variableName;
	varName.append(&variable[6]);
}


Script::Script(node* rootNode)
	:
	fRootNode(rootNode),
	fProcessed(false),
	fOrTriggers(0),
	fSender(NULL),
	fLastTrigger(NULL),
	fCurrentNode(NULL)
{
	if (rootNode == NULL)
		throw "Script::Script(): root node is NULL";

	fRootNode->next = NULL;
}


Script::~Script()
{
	_DeleteNode(fRootNode);
}


/* static */
void
Script::SetDebug(bool debug)
{
	sDebug = debug;
}


void
Script::Print() const
{
	node *nextNode = fRootNode;
	while (nextNode != NULL) {
		_PrintNode(nextNode);
		nextNode = nextNode->next;
	}
}


void
Script::Add(Script* script)
{
	if (script != NULL) {
		fRootNode->next = script->fRootNode;
		script->fRootNode = NULL;
	}
}


trigger_node*
Script::FindTriggerNode(node* start) const
{
	return static_cast<trigger_node*>(FindNode(BLOCK_TRIGGER, start));
}


action_node*
Script::FindActionNode(node* start) const
{
	return static_cast<action_node*>(FindNode(BLOCK_ACTION, start));
}


object_node*
Script::FindObjectNode(node* start) const
{
	return static_cast<object_node*>(FindNode(BLOCK_OBJECT, start));
}


int32
Script::FindMultipleObjectNodes(std::vector<object_node*>& list,
						node* start) const
{
	object_node* node = FindObjectNode(start);
	while (node != NULL) {
		list.push_back(node);
		node = static_cast<object_node*>(node->Next());
	}

	return list.size();
}


node*
Script::FindNode(block_type nodeType, node* start) const
{
	if (start == NULL)
		start = fRootNode;

	if (start->type == nodeType)
		return const_cast<node*>(start);

	node_list::const_iterator i;
	for (i = start->children.begin(); i != start->children.end(); i++) {
		node *n = FindNode(nodeType, *i);
		if (n != NULL)
			return n;
	}

	if (start->next != NULL)
		return FindNode(nodeType, start->next);

	return NULL;
}


Actor*
Script::FindObject(node* start) const
{
	object_node* objectNode = FindObjectNode(start);
	while (objectNode != NULL && Actor::IsDummy(objectNode)) {
		objectNode = static_cast<object_node*>(objectNode->Next());
	}

	if (objectNode == NULL)
		return NULL;

	/*if (sDebug)
		objectNode->Print();*/

	return GetObject((Actor*)fSender, objectNode);
}


bool
Script::Execute(bool& continuing)
{
	fLastTrigger = NULL;
	// for each CR block
	// for each CO block
	// check triggers
	// if any trigger
	// find response_set block
	// choose response
	// execute actions

	::node* nextScript = fRootNode;
	while (nextScript != NULL) {
		if (sDebug) {
			std::cout << "*** SCRIPT START: " << fSender->Name();
			std::cout << " ***" << std::endl;
#if 0
			if (!strcmp(fSender->Name(), "Amnish Soldier")) {
				Print();		
			}
#endif
		}
		
		bool foundContinue = continuing;
		// TODO: Find correct place
		::node* condRes = FindNode(BLOCK_CONDITION_RESPONSE, nextScript);
		while (condRes != NULL) {
			::node* condition = FindNode(BLOCK_CONDITION, condRes);
			while (condition != NULL) {
				if (sDebug)
					std::cout << "CONDITION" << std::endl;
				if (!_EvaluateConditionNode(condition))
					break;
					
				if (foundContinue) {
					std::cout << "CONTINUE(), EXIT SCRIPT" << std::endl;
					return true;
				}
					
				// Check action list
				if (!fSender->IsActionListEmpty()) {
					if (!fSender->IsInterruptable())
						break;
				}

				::node* responseSet = condition->Next();
				if (responseSet == NULL)
					break;
				if (sDebug)
					std::cout << "RESPONSE" << std::endl;
				if (!_HandleResponseSet(responseSet)) {
					// When the above method returns false, it means it encountered a CONTINUE
					// so the script should stop running.
					SetProcessed();
					foundContinue = true;
				} else {
					if (sDebug) {
						std::cout << "*** SCRIPT RETURNED " << fSender->Name();
						std::cout << " ***" << std::endl;
					}
					SetProcessed();
					return true;
				}
				condition = responseSet->Next();
			}
			condRes = condRes->Next();
		};
		if (sDebug) {
			std::cout << "*** SCRIPT END " << fSender->Name();
			std::cout << " ***" << std::endl;
		}
		nextScript = nextScript->next;
	}

	SetProcessed();

	return true;
}


Object*
Script::Sender()
{
	return fSender;
}


void
Script::SetSender(Object* object)
{
	fSender = object;
}


Object*
Script::ResolveIdentifier(const int id) const
{
	Script* thisCast = const_cast<Script*>(this);
	std::string identifier = IDTable::ObjectAt(id);
	if (identifier == "MYSELF")
		return thisCast->Sender();
	// TODO: Implement more identifiers
	if (identifier == "NEARESTENEMYOF") {
		Actor* actor = dynamic_cast<Actor*>(thisCast->Sender());
		if (actor == NULL)
			return NULL;
		return Core::Get()->GetNearestEnemyOf(actor);
	}
	// TODO: Move that one here ?

	if (identifier == "LASTTRIGGER")
		return LastTrigger();
#if 0
	if (identifier == "LASTATTACKEROF")
		return LastScriptRoundResults()->LastAttacker();
#endif
	std::cout << "ResolveIdentifier: UNIMPLEMENTED(" << id << ") = ";
	std::cout << identifier << std::endl;
	return NULL;
}


Actor*
Script::GetObject(Actor* source, object_node* node) const
{
	std::cout << "Script::GetObject() ";
	Actor* result = NULL;
	if (node->name[0] != '\0') {
		std::cout << "Specified name: " << node->name << std::endl; 
		result = Core::Get()->GetObject(node->name);
	} else if (node->identifiers[0] != 0) {
		std::cout << "Specified identifiers: " ;
		// If there are any identifiers, use those to get the object
		Actor* target = NULL;
		for (int32 id = 0; id < 5; id++) {
			const int identifier = node->identifiers[id];
			if (identifier == 0) {
				break;
			}
			std::cout << IDTable::ObjectAt(identifier) << ", ";
			target = dynamic_cast<Actor*>(ResolveIdentifier(identifier));
			/*if (source != NULL)
				source->Print();*/
		}
		std::cout << std::endl;
		// TODO: Filter using wildcards in node
		/*std::cout << "returned ";
		if (target != NULL)
			std::cout << target->Name() << std::endl;
		else
			std::cout << "NONE" << std::endl;*/
		result = target;
	} else {
		std::cout << "Specified wildcards (BUGGY):" << std::endl;
		// Otherwise use the other parameters
		Object* wildCard = Core::Get()->GetObject(node);
		if (wildCard != NULL)
			result = dynamic_cast<Actor*>(wildCard);
	}
	
	std::cout << "Script::GetObject() returned. ";
	if (result != NULL) {
		std::cout << "Found: " << std::endl;
		result->Print();
	} else
		std::cout << "Found NONE" << std::endl;
	return result;
}


Object*
Script::LastTrigger() const
{
	return fLastTrigger;
}


void
Script::SetProcessed()
{
	fProcessed = true;
}


bool
Script::Processed() const
{
	return fProcessed;
}


bool
Script::_EvaluateConditionNode(node* conditionNode)
{
	fOrTriggers = 0;

	trigger_node* trig = FindTriggerNode(conditionNode);
	bool blockEvaluation = true;
	while (trig != NULL) {
		if (fOrTriggers > 0) {
			blockEvaluation = _EvaluateTrigger(trig);
			if (blockEvaluation)
				break;
			fOrTriggers--;
		} else {
			blockEvaluation = _EvaluateTrigger(trig) && blockEvaluation;
			if (!blockEvaluation)
				break;
		}
		trig = static_cast<trigger_node*>(trig->Next());
	}
	if (sDebug) {
		std::cout << "SCRIPT: TRIGGER BLOCK returned ";
		std::cout << (blockEvaluation ? "TRUE" : "FALSE") << std::endl;
	}

	return blockEvaluation;
}


bool
Script::_EvaluateTrigger(trigger_node* trig)
{
	Actor* actor = dynamic_cast<Actor*>(fSender);
	//if (actor != NULL && actor->SkipConditions())
	//	return false;

	if (sDebug) {
		std::cout << "SCRIPT: TRIGGER ",
		std::cout << (trig->flags != 0 ? "!" : "");
		std::cout << IDTable::TriggerAt(trig->id) << " (";
		std::cout << std::dec << trig->id << " ";
		std::cout << std::hex << trig->id << ")";
		std::cout << std::endl;
		trig->Print();
	}

	Core* core = Core::Get();
	bool returnValue = false;
	try {
		switch (trig->id) {
			case 0x0002:
			{
				/* 0x0002 AttackedBy(O:Object*,I:Style*AStyles)
				 * Returns true only if the active CRE was attacked in the style
				 * specified (not necessarily hit) or had an offensive spell cast
				 * on it by the specified object in the last script round.
				 * The style parameter is non functional - this trigger is triggered
				 * by any attack style.
				 */
				object_node* node = FindObjectNode(trig);
				if (node == NULL)
					break;
				node->Print();
				if (Core::Get()->LastRoundResults()->WasActorAttackedBy(actor, node))
					returnValue = true;
				
				break;
			}
			case 0x0020:
			{
				// HitBy
				// Returns true on first Script launch, IOW initial area load
				if (!Processed()) {
					returnValue = true;
					break;
				}
				//object_node* object = FindObjectNode(trig);
				//returnValue = Object::CheckIfNodeInList(object,
				//		fSender->LastScriptRoundResults()->Hitters());
				break;
			}
			case 0x0022:
			{
				/* TimerExpired(I:ID*) */
				std::ostringstream stringStream;
				stringStream << fSender->Name() << " " << trig->parameter1;
				printf("TimerExpired %s\n", stringStream.str().c_str());
				GameTimer* timer = GameTimer::Get(stringStream.str().c_str());
				if (timer != NULL && timer->Expired()) {
					returnValue = true;
				}
				break;
			}
			case 0x002F:
			{
				/* 0x002F Heard(O:Object*,I:ID*SHOUTIDS)
				Returns true only if the active CRE was within 30 feet
				of the specified object and the specified object shouted
				the specified number (which does not have to be in
				SHOUTIDS.ids) in the last script round.
				NB. If the object is specified as a death variable,
				the trigger will only return true if the corresponding
				object shouting also has an Enemy-Ally flag of NEUTRAL. */
#if 0
				Object* object = FindObject(trig);
				if (object != NULL && core->Distance(fSender, object) <= 30
						&& object->LastScriptRoundResults()->Shouted()
						== trig->parameter1) {
					returnValue = true;
				}
#endif
				break;
			}
			case 0x0036:
			{
				/*0x0036 OnCreation()
				Returns true if the script is processed for the first time this session,
				e.g. when a creature is created (for CRE scripts) or when the player
				enters an area (for ARE scripts).*/
				returnValue = !Processed();
				break;
			}
			case 0x004c:
			{
				// Entered(O:Object)
				//object_node* node = FindObjectNode(trig);
				//Region* region = dynamic_cast<Region*>(fSender);
				//std::vector<std::string>::const_iterator i;

				/*Object* object = Object::GetMatchingObjectFromList(
										region->
										LastScriptRoundResults()->
										EnteredActors(), node);*/
				//if (object != NULL) {
				//	fLastTrigger = object;
				//	returnValue = true;
				//}
				break;
			}
			case 0x0052:
			{
				/* OPENED(O:OBJECT*) (82 0x52) */
				object_node* objectNode = FindObjectNode(trig);
				if (objectNode == NULL)
					break;
				break;
				// TODO: We assume this is a door, but also
				// containers can be opened/closed
				/*Door* door = dynamic_cast<Door*>(fSender);
				if (door == NULL)
					break;
				if (!door->Opened())
					break;*/
#if 0
				Object* object = core->GetObject(
						door->CurrentScriptRoundResults()->fOpenedBy.c_str());
				if (object != NULL)
					returnValue = object->MatchNode(objectNode);
#endif
				break;
			}
			case 0x0070:
			{
				/* 0x0070 Clicked(O:Object*)
				 *	Only for trigger regions.
				 *	Returns true if the specified object
				 *	clicked on the trigger region running this script.
				 */
				object_node* objectNode = FindObjectNode(trig);
				Actor* clicker = core->LastRoundResults()->GetActorWhoClickedObject(fSender);
				if (clicker != NULL) {
					objectNode->Print();
					returnValue = clicker->MatchNode(objectNode);
				}
				// TODO: When to set this, other than now ?
				if (returnValue)
					fLastTrigger = fSender;

				break;
			}
			case 0x00B1:
			{
				/* ACTION TRIGGERACTIVATION(O:OBJECT*,I:STATE*BOOLEAN)(177 0xb1) */
				object_node* objectNode = FindObjectNode(trig);
				objectNode->Print();
				break;
			}
			case 0x400A:
			{
				//ALIGNMENT(O:OBJECT*,I:ALIGNMENT*Align) (16395 0x400A)
				Actor* object = FindObject(trig);
				if (object != NULL)
					returnValue = object->IsAlignment(trig->parameter1);
				break;
			}
			case 0x400B:
			{
				//ALLEGIANCE(O:OBJECT*,I:ALLEGIENCE*EA) (16395 0x400b)
				Actor* object = FindObject(trig);
				if (object != NULL)
					returnValue = object->IsEnemyAlly(trig->parameter1);
				break;
			}
			case 0x400C:
			{
				/*0x400C Class(O:Object*,I:Class*Class)*/
				Actor* object = FindObject(trig);
				if (object != NULL)
					returnValue = object->IsClass(trig->parameter1);
				break;
			}
			case 0x400E:
			{
				/* GENERAL(O:OBJECT*,I:GENERAL*GENERAL) (16398 0x400e)*/
				Actor* object = FindObject(trig);
				if (object != NULL) {
					returnValue = object->IsGeneral(trig->parameter1);
					if (sDebug) {
						std::cout << "object: " << IDTable::GeneralAt(object->CRE()->General());
						std::cout << " vs " << IDTable::GeneralAt(trig->parameter1) << std::endl;				
					}				
				}
				break;
			}
			case 0x400F:
			{
				/*0x400F Global(S:Name*,S:Area*,I:Value*)
				Returns true only if the variable with name 1st parameter
				of type 2nd parameter has value 3rd parameter.*/
				std::string variableScope;
				std::string variableName;
				VariableGetScopeName(trig->string1, variableScope, variableName);
				int32 variableValue = 0;
				if (variableScope.compare("LOCALS") == 0) {
					variableValue = fSender->Vars().Get(variableName.c_str());
				} else {
					// TODO: Check for AREA variables, currently we
					// treat AREA variables as global variables
					variableValue = Core::Get()->Vars().Get(trig->string1);
				}
				returnValue = variableValue == trig->parameter1;
				break;
			}
			case 0x4017:
			{
				// Race()
				Actor* object = FindObject(trig);
				if (object != NULL)
					returnValue = object->IsRace(trig->parameter1);
				break;
			}

			case 0x4018:
			{
				/* 0x4018 Range(O:Object*,I:Range*)
				Returns true only if the specified object
				is within distance given (in feet) of the active CRE. */
				Actor* object = FindObject(trig);
				if (object != NULL)
					returnValue = core->Distance(object, fSender) <= trig->parameter1;
				break;
			}

			case 0x401C:
			{
				/* See(O:Object*)
				 * Returns true only if the active CRE can see
				 * the specified object which must not be hidden or invisible.
				 */
				Actor* object = FindObject(trig);
				if (object != NULL)
					returnValue = core->See(actor, object);
				break;
			}
			case 0x401E:
			{
				/* Time(I:Time*Time)
				 * Returns true only if the period of day matches the period
				 * in the 2nd parameter (taken from Time.ids).
				 * Hours are offset by 30 minutes, e.g. Time(1) is true
				 * between 00:30 and 01:29.
				 */
				if (GameTimer::HourOfDay() == trig->parameter1)
					returnValue = true;
				break;
			}
			case 0x4023:
				/* 0x4023 True() */
				returnValue = true;
				break;
			case 0x4027:
			{
				//DELAY(I:DELAY*) (16423 0x4027)
				// TODO: Implement
				
				returnValue = true;
				break;
			}
			case 0x402b:
			{
				/* ACTIONLISTEMPTY() (16427 0x402b)*/
				returnValue = actor->IsActionListEmpty();
				break;
			}
			case 0x4034:
			{
				/*0x4034 GlobalGT(S:Name*,S:Area*,I:Value*)
				See Global(S:Name*,S:Area*,I:Value*) except the variable
				must be greater than the value specified to be true.
				*/
				returnValue = core->Vars().Get(trig->string1) > trig->parameter1;
				break;
			}
			case 0x4035:
			{	/*
				0x4035 GlobalLT(S:Name*,S:Area*,I:Value*)
				As above except for less than. */
				returnValue = core->Vars().Get(trig->string1) < trig->parameter1;
				break;
			}
			case 0x4037:
			{
				/* StateCheck(O:Object*,I:State*State)
				 * Returns true only if the specified object
				 * is in the state specified.
				 */
				Actor* object = FindObject(trig);
				if (object != NULL)
					returnValue = object->IsState(trig->parameter1);
				break;
			}
			case 0x4039:
			{
				/* NUMBEROFTIMESTALKEDTO(I:NUM*) (16441 0x4039) */
				if (actor->NumTimesTalkedTo() == (uint32)trig->parameter1)
					returnValue = true;
				break;
			}
			case 0x4040:
			{
				/* GlobalTimerExpired(S:Name*,S:Area*) (16448 0x4040) */
				// TODO: Merge Name and Area before passing them to the
				// Timer::Get() method (also below)
				std::string timerName;
				timerName.append(trig->string2).append(trig->string1);
				GameTimer* timer = GameTimer::Get(timerName.c_str());
				returnValue = timer != NULL && timer->Expired();
				break;
			}
			case 0x4041:
			{
				/* GlobalTimerNotExpired(S:Name*,S:Area*) */
				std::string timerName;
				timerName.append(trig->string2).append(trig->string1);
				GameTimer* timer = GameTimer::Get(timerName.c_str());
				returnValue = timer == NULL || !timer->Expired();
				break;
			}
			case 0x4043:
			{
				// InParty
				const Actor* actor = FindObject(trig);
				if (actor != NULL)
					returnValue = Game::Get()->Party()->HasActor(actor);
				break;
			}
			case 0x4051:
			{
				/*
				 * 0x4051 Dead(S:Name*)
				 * Returns only true if the creature with the specified script name
				 * has its death variable set to 1. Not every form of death sets this,
				 * but most do. So it's an almost complete test for death.
				 * The creature must have existed for this to be true.
				 * Note that SPRITE_IS_DEAD variables are not set if the creature is
				 * killed by a neutral creature.
				 */
				/*Script* actorScript = fScripts[trig->string1];
				if (actorScript != NULL) {
					// TODO: More NULL checking
					const char* deathVariable = actorScript->Target()->CRE()->DeathVariable();
					returnValue = fVariables[deathVariable] == 1;
				}*/
				break;
			}
			case 0x4063:
			{
				/*INWEAPONRANGE(O:OBJECT*) (16483 0x4063) */
				int range = 40;
				// TODO: Check weapon range
				Actor* object = FindObject(trig);
				if (object != NULL)
					returnValue = core->Distance(object, fSender) <= range;
				break;
			}
			case 0x4068:
			{
				/* TimeGT(I:Time*Time)
				 * Returns true only if the current time is greater than
				 * that specified. Hours are offset by 30 minutes,
				 * e.g. TimeGT(1) is true between 01:30 and 02:29.
				 */
				// TODO: Offset hour
				if (GameTimer::HourOfDay() > trig->parameter1)
					returnValue = true;
				break;
			}
			case 0x4069:
			{
				//TIMELT(I:TIME*TIME) (16489 0x4069)
				// TODO: Offset hour
				if (GameTimer::HourOfDay() < trig->parameter1)
					returnValue = true;
				break;
			}
			case 0x4074:
			{
				/*
				 * 0x4074 Detect(O:Object*)
				 * Returns true if the the specified object
				 * is detected by the active CRE in any way
				 * (hearing or sight). Neither Move Silently
				 * and Hide in Shadows prevent creatures
				 * being detected via Detect().
				 * Detect ignores Protection from Creature
				 * type effects for static objects.
				 */
				// TODO: This is valid for Regions scripts. What is the "Active CRE" ?
				if (actor == NULL)
					break;
				Object* object = GetObject(actor, FindObjectNode(trig));
				if (object != NULL)
					returnValue = core->See(actor, object);
					// || core->Hear(fSender, object);
				break;
			}
			case 0x4076:
			{
				/*
				 * 0x4076 OpenState(O:Object*,I:Open*BOOLEAN)
				 * NT Returns true only if the open state of the specified door
				 * matches the state specified in the 2nd parameter.
				 */
				object_node* doorObj = FindObjectNode(trig);
				Door *door = dynamic_cast<Door*>(
								core->GetObject(doorObj->name));
				if (door != NULL) {
					bool openState = trig->parameter1 == 1;
					returnValue = door->Opened() == openState;
				}
				break;
			}
			case 0x407D:
			{
				/* ISOVERME(O:Object*)
				 * Only for trigger regions. Returns true only if the specified
				 * object is over the trigger running the script
				 */
				Region* region = dynamic_cast<Region*>(fSender);
				if (region == NULL)
					break;
				object_node* objectNode = FindObjectNode(trig);
				if (objectNode != NULL) {
					// TODO: won't work if the object is generic and not specified					
					Actor* actor = core->GetObject(objectNode);
					if (actor != NULL)
						returnValue = region->Contains(actor->Position());
				}

				break;
			}
			case 0x407E:
			{
				/* 0x407E AreaCheck(S:ResRef*)
				 * Returns true only if the active CRE
				 * is in the area specified.
				 */
				// TODO: We only check the active area
				returnValue = !strcmp(core->CurrentRoom()->Name(), trig->string1);
				break;
			}
			case 0x4086:
			{
				// AREATYPE(I:NUMBER*AREATYPE) (16518 0x4086)

				// TODO: We only check the active area
				//const uint16 areaType = RoomContainer::Get()->AREA()->Type();
				//returnValue = areaType & trig->parameter1;
				break;
			}
			case 0x4089:
			{
				/*0x4089 OR(I:OrCount*)*/
				fOrTriggers = trig->parameter1;
				returnValue = true;
				break;
			}
			case 0x401b:
			{
				/* REPUTATIONLT(O:OBJECT*,I:REPUTATION*) (16411 0x401b) */
				Actor* actor = FindObject(trig);
				if (actor != NULL) {
					returnValue = actor->CRE()->Reputation() < trig->parameter1;
				}
				break;
			}
			default:
			{
				if (sDebug) {
					std::cout << "SCRIPT: UNIMPLEMENTED TRIGGER!!!" << std::endl;
				}
				break;
			}
		}
	} catch (...) {
		std::cerr << "SCRIPT: EvaluateTrigger() caught exception" << std::endl;
	}
	if (trig->flags != 0)
		returnValue = !returnValue;
	return returnValue;
}


bool
Script::_HandleResponseSet(node* responseSet)
{
	response_node* responses[5];
	response_node* response = static_cast<response_node*>(
			FindNode(BLOCK_RESPONSE, responseSet));
	int i = 0;
	int totalChance = 0;
	while (response != NULL) {
		responses[i] = response;
		totalChance += responses[i]->probability;
		response = static_cast<response_node*>(response->Next());
		i++;
	}
	
	for (int p = 0; p < i; p++) {
		std::cout << "response " << p << ": probability ";
		std::cout << std::dec << responses[p]->probability << std::endl;
	}
	// TODO: Fix this and take the probability into account
	int randomResponse = Core::RandomNumber(0, i);
	action_node* action = FindActionNode(responses[randomResponse]);
	// More than one action
	while (action != NULL) {
		// When _HandleAction() returns false,
		// it means it's a CONTINUE() action.
		if (!_HandleAction(action))
			return false;
		action = static_cast<action_node*>(action->Next());
	}
	return true;
}


bool
Script::_HandleAction(action_node* act)
{
	if (sDebug) {
		std::cout << "SCRIPT: ACTION ";
		std::cout << IDTable::ActionAt(act->id);
		std::cout << "(" << std::dec << (int)act->id << std::hex << " 0x" << (int)act->id << ")";
		std::cout << std::endl; 
		act->Print();
	}
	Core* core = Core::Get();
	Actor* thisActor = dynamic_cast<Actor*>(fSender);

	switch (act->id) {
		case 0x03:
		{
			/* Attack(O:Target*) */
			Actor* targetActor = FindObject(act);
			if (targetActor != NULL) {
				if (thisActor != NULL) {
					Attack* attackAction = new Attack(thisActor, targetActor);
					thisActor->AddAction(attackAction);
				}
			}
			break;
		}
		case 0x07:
		{
			/* CreateCreature(S:NewObject*,P:Location*,I:Face*) */
			// TODO: If point is (-1, -1) we should put the actor near
			// the active creature. Which one is the active creature?
			IE::point point = act->where;
			if (point.x == -1 && point.y == -1) {
				point = Game::Get()->Party()->ActorAt(0)->Position();
				point.x += Core::RandomNumber(-20, 20);
				point.y += Core::RandomNumber(-20, 20);
			}
			// TODO: should use "core->ActiveActor()" instead ?
			std::cout << "active creature: " << fSender->Name() << std::endl;
			Actor* actor = new Actor(act->string1, point, act->integer1);
			core->AddActorToCurrentArea(actor);
			//core->SetActiveActor(actor);
			break;
		}
		case 0x8:
		{
			/*	DIALOGUE(O:OBJECT*) (8 0x8) */
			Actor* actor = FindObject(act);
			if (actor != NULL) {
				Dialogue* dialogueAction = new Dialogue(thisActor, actor);
				thisActor->AddAction(dialogueAction);
			}
			break;
		}
		case 0xA:
		{
			/* ENEMY() (10 0xa) */
			uint32 id = IDTable::EnemyAllyValue("ENEM");
			thisActor->SetEnemyAlly(id);
			break;
		}
		case 22:
		{
			// MoveToObject
			Object* object = FindObject(act);
			if (object != NULL) {
				WalkTo* walkTo = new WalkTo(thisActor, object->Position());
				thisActor->AddAction(walkTo);
			}
			break;
		}
		case 23:
		{
			// MoveToPoint
			Actor* actor = dynamic_cast<Actor*>(fSender);
			if (actor != NULL) {
				WalkTo* walkTo = new WalkTo(actor, act->where);
				actor->AddAction(walkTo);
				actor->StopCheckingConditions();
			}
			break;
		}
		case 29:
		{
			/* RunAwayFrom(O:Creature*,I:Time*) */
			Actor* targetActor = dynamic_cast<Actor*>(FindObject(act));
			if (targetActor != NULL && thisActor != NULL) {
				RunAwayFrom* run = new RunAwayFrom(thisActor, targetActor);
				thisActor->AddAction(run);
			}
			break;
		}
		case 36:
		{
			/*
			 * 36 Continue()
			 * This action instructs the script parser to continue looking
			 * for actions in the active creatures action list.
			 * This is mainly included in scripts for efficiency.
			 * Continue should also be appended to any script blocks added
			 * to the top of existing scripts, to ensure correct functioning
			 * of any blocks which include the OnCreation trigger.
			 * Continue may prevent actions being completed until the script
			 * parser has finished its execution cycle. Continue() must be
			 * the last command in an action list to function correctly.
			 * Use of continue in a script block will cause the parser
			 * to treater subsequent empty response blocks as though they
			 * contained a Continue() command - this parsing can be stopped
			 * by including a NoAction() in the empty response block.
			 */
			// by returning false, we instruct the caller to stop
			// executing the script
			return false;
		}
		case 49:
		{
			/* MOVEVIEWPOINT(P:TARGET*,I:SCROLLSPEED*SCROLL)(49 0x31) */
			Action* action = new MoveViewPoint(fSender, act->where, act->integer1);
			core->AddGlobalAction(action);
			break;
		}
		case 61:
		{
			/* 61 StartTimer(I:ID*,I:Time*)
			This action starts a timer local to the active creature.
			The timer is measured in seconds, and the timer value is
			not saved in save games. The timer is checked with the
			TimerExpired trigger.*/

			// TODO: We should add the timer local to the active creature,
			// whatever that means
			if (fSender == NULL)
				printf("NULL TARGET\n");
			std::ostringstream stringStream;
			stringStream << fSender->Name() << " " << act->integer1;

			GameTimer::Add(stringStream.str().c_str(), act->integer2);

			break;
		}
		case 63:
		{
			/* WAIT(I:TIME*)(63 0x3f) */
			Wait* wait = new Wait(fSender, act->integer1 * 15);
			fSender->AddAction(wait);
			break;
		}
		case 0x54:
		{
			/* 84 (0x54) FACE(I:DIRECTION) */
			if (thisActor != NULL) {
				ChangeOrientationExtAction* action = new ChangeOrientationExtAction(thisActor, act->integer1);
				thisActor->AddAction(action);
			}				
			break;
		}
		case 85:
		{
			/* 85 RandomWalk */
			if (thisActor != NULL && thisActor->IsInterruptable())
				core->RandomWalk(thisActor);
			break;
		}
		case 86:
		{
			/* 86 SetInterrupt(I:State*Boolean) */
			fSender->AddAction(new SetInterruptableAction(fSender, act->integer1 == 1));
			break;
		}
		case 0x1E:
		{
			std::string variableScope;
			std::string variableName;
			VariableGetScopeName(act->string1, variableScope, variableName);
			if (variableScope.compare("LOCALS") == 0) {
				if (fSender != NULL)
					fSender->Vars().Set(variableName.c_str(),
							act->integer1);
			} else {
				// TODO: Check for AREA variables
				core->Vars().Set(act->string1, act->integer1);
			}
			break;
		}
		case 0x53:
		{
			/* 83 SmallWait(I:Time*) */
			// TODO: The time is probably wrong
			//
			Wait* wait = new Wait(fSender, act->integer1);
			fSender->AddAction(wait);
			break;
		}

		case 106:
		{
			/* Shout */
			// Check if target is silenced
			if (thisActor != NULL)
				thisActor->Shout(act->integer1);
			break;
		}
		case 0x64:
		{
			/* 100 RandomFly */
			if (thisActor != NULL && thisActor->IsInterruptable())
				core->RandomFly(thisActor);
			break;
		}
		case 0x65:
		{
			/* 101 FlyToPoint(Point, Time) */
			if (thisActor != NULL && thisActor->IsInterruptable()) {
				FlyTo* flyTo = new FlyTo(thisActor, act->where, act->integer1);
				thisActor->AddAction(flyTo);
			}
			break;
		}
		case 0x6d:
		{	
			// INCREMENTGLOBAL(S:NAME*,S:AREA*,I:VALUE*) (109 0x6d)
			std::string variableScope;
			std::string variableName;
			VariableGetScopeName(act->string1, variableScope, variableName);			
			int32 value = core->Vars().Get(act->string1);
			core->Vars().Set(act->string1, value + act->integer1);
			break;		
		}		
		case 0x6f:
		{
			/* DESTROYSELF() (111 0x6f) */
			// TODO: Delete it for real
			Actor* activeActor = core->ActiveActor();
			if (activeActor != NULL) {
				std::cout << "DESTROY SELF" << std::endl;
				activeActor->DestroySelf();
				core->SetActiveActor(NULL);
			}
			break;
			//return false;
		}
		case 0x73:
		{
			/* SETGLOBALTIMER(S:NAME*,S:AREA*,I:TIME*GTIMES) (115 0x73)*/
			std::string timerName;
			// TODO: We append the timer name to the area name,
			// check if it's okay
			timerName.append(act->string2).append(act->string1);
			GameTimer::Add(timerName.c_str(), act->integer1);
			break;
		}
		case 120:
		{
			/* STARTCUTSCENE(S:CUTSCENE*)(120 0x78) */
			core->StartCutscene(act->string1);
			break;
		}
		case 121:
		{
			/* STARTCUTSCENEMODE()(121 0x79) */
			// TODO: add as an action ?
			core->StartCutsceneMode();
			break;
		}
		case 0x7f:
		{
			/* CUTSCENEID(O:OBJECT*)(127 0x7f) */
			// TODO: Should be correct			
			Actor* actor = FindObject(act);
			SetSender(actor);
			break;
		}
		case 0x97:
		{
			/* 151 DisplayString(O:Object*,I:StrRef*)
			 * This action displays the strref specified by the StrRef parameter
			 * in the message window, attributing the text to
			 * the specified object.
			 */
			core->DisplayMessage(act->integer1);
			break;
		}
		case 0xA7:
		{
			core->PlayMovie(act->string1);
			break;
		}
		case 134:
		{
			/* AttackReevaluate(O:Target*,I:ReevaluationPeriod*)
			 *  (134 0x86)
			 */
			Actor* targetActor = dynamic_cast<Actor*>(FindObject(act));
			if (thisActor != NULL && targetActor != NULL) {
				IE::point point = targetActor->NearestPoint(thisActor->Position());
				WalkTo* walkToAction = new WalkTo(thisActor, point);
				thisActor->AddAction(walkToAction);
				Attack* attackAction = new Attack(thisActor, targetActor);
				thisActor->AddAction(attackAction);
			}

			break;
		}
		case 143:
		{
			/* OPENDOOR(O:OBJECT*)(143 0x8f) */
			Door* door = dynamic_cast<Door*>(FindObject(act));
			if (door != NULL)
				fSender->AddAction(new OpenDoor(fSender, door));
			break;
		}
		case 198: // 0xc6
		{
			/* STARTDIALOGNOSET(O:OBJECT*) (198 0xc6) */
			// TODO: Implement more correctly.
			Actor* actor = dynamic_cast<Actor*>(FindObject(act));
			if (actor != NULL) {
				Dialogue* dialogueAction = new Dialogue(thisActor, actor);
				thisActor->AddAction(dialogueAction);
			}
			break;
		}
		case 202:
		{	
			/* FADETOCOLOR(P:POINT*,I:BLUE*) (202 0xca) */
			int numUpdates = act->where.x;
			FadeColorAction* action = new FadeColorAction(fSender,
				numUpdates, FadeColorAction::TO_BLACK);
			core->AddGlobalAction(action);
			break;		
		}
		case 203:
		{
			/* FADEFROMCOLOR(P:POINT*,I:BLUE*)(203 0xcb) */
			int numUpdates = act->where.x;
			FadeColorAction* action = new FadeColorAction(fSender,
				numUpdates, FadeColorAction::FROM_BLACK);
			core->AddGlobalAction(action);
			break;
		}
		case 207:
		{
			/* 207 MoveToPointNoInterrupt(P:Point*)
			 * This action causes the active creature to move to the specified coordinates.
			 * The action will update the position of creatures as stored in ARE files
			 * (first by setting the coordinates of the destination point, then by setting
			 * the coordinates of the current point once the destination is reached).
			 * Conditions are not checked until the destination point is reached.*/
			Actor* actor = dynamic_cast<Actor*>(fSender);
			if (actor != NULL) {
				WalkTo* walkTo = new WalkTo(actor, act->where);
				thisActor->AddAction(walkTo);
				thisActor->StopCheckingConditions();
			}
			break;
		}
		case 228: // 0xe4
		{
			/* CreateCreatureImpassable(S:NewObject*,P:Location*,I:Face*) (228 0xe4) */
			/* This action creates the specified creature
			 * on a normally impassable surface (e.g. on a wall,
			 * on water, on a roof). */
			Actor* actor = new Actor(act->string1, act->where, act->integer1);
			std::cout << "Created actor " << act->string1 << " on ";
			std::cout << act->where.x << ", " << act->where.y << std::endl;
			actor->SetDestination(act->where);
			core->AddActorToCurrentArea(actor);
			//core->SetActiveActor(actor);
			break;
		}
		case 286: // 0x11e
		{
			/* HIDEGUI */
			GUI::Get()->Hide();
			break;
		}
		case 287:
		{
			/* UNHIDEGUI()(287 0x11f) */
			GUI::Get()->Show();
			break;
		}
		default:
			if (sDebug) {
				std::cout << "SCRIPT: UNIMPLEMENTED ACTION!!!" << std::endl;
			}
			break;
	}

	return true;
}


void
Script::_PrintNode(node* n) const
{
	PrintIndentation();
	printf("<%s>", n->header);
	n->Print();
	node_list::iterator c;
	IndentMore();
	for (c = n->children.begin(); c != n->children.end(); c++) {
		_PrintNode(*c);
	}
	IndentLess();
	PrintIndentation();
	printf("<%s/>\n", n->header);
}


void
Script::_DeleteNode(::node* node)
{
	if (node != NULL) {
		if (node->next != NULL)
			_DeleteNode(node->next);
		delete node;
	}
}


// node
/* static */
node*
node::Create(int type, const char *string)
{
	node* newNode = NULL;
	switch (type) {
		case BLOCK_TRIGGER:
			newNode = new trigger_node;
			break;
		case BLOCK_OBJECT:
			newNode = new object_node;
			break;
		case BLOCK_ACTION:
			newNode = new action_node;
			break;
		case BLOCK_RESPONSE:
			newNode = new response_node;
			break;
		default:
			newNode = new node;
			break;
	}
	if (newNode != NULL) {
		newNode->type = type;
		strcpy(newNode->header, string);
	}
	return newNode;
}


// node
node::node()
	:
	type(BLOCK_UNKNOWN),
	parent(NULL),
	next(NULL),
	closed(false)

{
	value[0] = '\0';
}


node::~node()
{
	node_list::iterator i;
	for (i = children.begin(); i != children.end(); i++)
		delete (*i);
}


void
node::AddChild(node* child)
{
	child->parent = this;
	child->next = NULL;
	if (children.size() > 0) {
		std::vector<node*>::reverse_iterator i = children.rbegin();
		(*i)->next = child;
	}
	children.push_back(child);
}


node*
node::Next() const
{
	return next;
}


void
node::Print() const
{
	//printf("header: %s\n", header);
	//printf("value: %s\n", value);
}


bool
operator==(const node &a, const node &b)
{
	return false;
}


// trigger
trigger_node::trigger_node()
{
}


void
trigger_node::Print() const
{
	printf("id:%d, flags: %d, parameter1: %d, parameter2: %d, %s %s\n",
		id, flags, parameter1, parameter2, string1, string2);
}


// object
object_node::object_node()
{
}


void
object_node::Print() const
{
	std::cout << "Object: ";
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
		std::cout << "alignment: " << IDTable::AlignmentAt(alignment) << " (" << alignment << "), " << std::endl;
	for (int32 i = 4; i >= 0; i--) {
		if (identifiers[i] != 0) {
			std::cout << IDTable::ObjectAt(identifiers[i]);
			if (i != 0)
				std::cout << " -> ";
		}
	}
	std::cout << std::endl;
	if (Core::Get()->Game() == GAME_TORMENT)
		std::cout << "point: " << point.x << ", " << point.y << std::endl;
	if (name[0] != '\0')
		 std::cout << "name: *" << name << "*" << std::endl;
}


// action
action_node::action_node()
{
}


void
action_node::Print() const
{
	printf("SCRIPT: id: %d, parameter: %d, point: (%d, %d), %d, %d, %s, %s\n",
			id, integer1, where.x, where.y, integer2, integer3, string1, string2);
}


//response
response_node::response_node()
{
}


void
response_node::Print() const
{
	printf("probability: %d\n", probability);
}
