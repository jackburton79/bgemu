#include "Action.h"
#include "Actor.h"
#include "AreaResource.h"
#include "Core.h"
#include "CreResource.h"
#include "Door.h"
#include "Game.h"
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

#define DEBUG_SCRIPTS 1

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


Script::Script(node* rootNode)
	:
	fRootNode(rootNode),
	fProcessed(false),
	fOrTriggers(0),
	fTarget(NULL),
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
#if DEBUG_SCRIPTS
	objectNode->Print();
#endif
	return Core::Get()->GetObject((Actor*)fTarget.Target(), objectNode);
}


bool
Script::Execute()
{
	// TODO: Move this to a better place
	if (fTarget != NULL)
		fTarget.Target()->NewScriptRound();

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
#if DEBUG_SCRIPTS
		std::cout << "*** SCRIPT START: " << fTarget.Target()->Name();
		std::cout << " ***" << std::endl;
#endif
		::node* condRes = FindNode(BLOCK_CONDITION_RESPONSE, nextScript);
		while (condRes != NULL) {
			::node* condition = FindNode(BLOCK_CONDITION, condRes);
			while (condition != NULL) {
				if (!_CheckTriggers(condition))
					break;

				::node* responseSet = condition->Next();
				if (responseSet == NULL)
					break;

				if (!_ExecuteActions(responseSet)) {
					// TODO: When the above method returns false,
					// The script should stop running.
					return false;
				}

				condition = responseSet->Next();
			}
			condRes = condRes->Next();
		};
#if DEBUG_SCRIPTS
		std::cout << "*** SCRIPT END " << fTarget.Target()->Name();
		std::cout << " ***" << std::endl;
#endif
		nextScript = nextScript->next;
	}

	SetProcessed();

	return true;
}


void
Script::SetTarget(Object* object)
{
	fTarget = object;
}


Object*
Script::LastTrigger() const
{
	return fLastTrigger.Target();
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
Script::_CheckTriggers(node* conditionNode)
{
	fOrTriggers = 0;

	trigger_node* trig = FindTriggerNode(conditionNode);
	bool evaluation = true;
	while (trig != NULL) {
		if (fOrTriggers > 0) {
			evaluation = _EvaluateTrigger(trig);
			if (evaluation)
				break;
			fOrTriggers--;
		} else {
			evaluation = _EvaluateTrigger(trig) && evaluation;
			if (!evaluation)
				break;
		}
		trig = static_cast<trigger_node*>(trig->Next());
	}
	return evaluation;
}


bool
Script::_EvaluateTrigger(trigger_node* trig)
{
	Actor* actor = dynamic_cast<Actor*>(fTarget.Target());
	if (actor != NULL && actor->SkipConditions())
		return false;

#if DEBUG_SCRIPTS
	printf("SCRIPT: %s%s (%d 0x%x) ? ",
			trig->flags != 0 ? "!" : "",
			IDTable::TriggerAt(trig->id).c_str(),
			trig->id, trig->id);
	trig->Print();
#endif

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
				if (object != NULL)
					returnValue = object->IsGeneral(trig->parameter1);
				break;
			}
			case 0x400F:
			{
				/*0x400F Global(S:Name*,S:Area*,I:Value*)
				Returns true only if the variable with name 1st parameter
				of type 2nd parameter has value 3rd parameter.*/
				std::string variableScope;
				variableScope.append(trig->string1, 6);
				std::string variableName;
				variableName.append(&trig->string1[6]);

				int32 variableValue = 0;
				if (variableScope.compare("LOCALS") == 0) {
					variableValue = fTarget.Target()->GetVariable(variableName.c_str());
				} else {
					// TODO: Check for AREA variables, currently we
					// treat AREA variables as global variables
					variableValue = Core::Get()->GetVariable(variableName.c_str());
				}
				returnValue = variableValue == trig->parameter1;
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
				//		fTarget->LastScriptRoundResults()->Hitters());
				break;
			}
			case 0x0022:
			{
				/* TimerExpired(I:ID*) */
				std::ostringstream stringStream;
				stringStream << fTarget.Target()->Name() << " " << trig->parameter1;
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
				if (object != NULL && core->Distance(fTarget.Target(), object) <= 30
						&& object->LastScriptRoundResults()->Shouted()
						== trig->parameter1) {
					returnValue = true;
				}
#endif
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
					returnValue = core->Distance(object, fTarget.Target()) <= trig->parameter1;
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
				returnValue = core->GetVariable(trig->string1) > trig->parameter1;
				break;
			}
			case 0x4035:
			{	/*
				0x4035 GlobalLT(S:Name*,S:Area*,I:Value*)
				As above except for less than. */
				returnValue = core->GetVariable(trig->string1) < trig->parameter1;
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
			case 0x52:
			{
				/* OPENED(O:OBJECT*) (82 0x52) */
				object_node* objectNode = FindObjectNode(trig);
				if (objectNode == NULL)
					break;
				// TODO: We assume this is a door, but also
				// containers can be opened/closed
				Door* door = dynamic_cast<Door*>(fTarget.Target());
				if (door == NULL)
					break;
				if (!door->Opened())
					break;
#if 0
				Object* object = core->GetObject(
						door->CurrentScriptRoundResults()->fOpenedBy.c_str());
				if (object != NULL)
					returnValue = object->MatchNode(objectNode);
#endif
				break;
			}
			case 0x4063:
			{
				/*INWEAPONRANGE(O:OBJECT*) (16483 0x4063) */
				int range = 40;
				// TODO: Check weapon range
				Actor* object = FindObject(trig);
				if (object != NULL)
					returnValue = core->Distance(object, fTarget.Target()) <= range;

				break;
			}

#if 0
			case 0x4068:
			{
				/* TimeGT(I:Time*Time)
				 * Returns true only if the current time is greater than
				 * that specified. Hours are offset by 30 minutes,
				 * e.g. TimeGT(1) is true between 01:30 and 02:29.
				 */
				break;
			}

			case 0x4069:
			{
				//TIMELT(I:TIME*TIME) (16489 0x4069)
				break;
			}
#endif
			case 0x0070:
			{
				/* 0x0070 Clicked(O:Object*)
				 *	Only for trigger regions.
				 *	Returns true if the specified object
				 *	clicked on the trigger region running this script.
				 */
#if 0
				object_node* objectNode = FindObjectNode(trig);
				//returnValue = fTarget.Target()->LastScriptRoundResults()->Clicker()
				//		->MatchNode(objectNode);
				//objectNode->Print();
				//fTarget.Target()->LastScriptRoundResults()->Clicker()->Print();

				// TODO: When to set this, other than now ?
				if (returnValue)
					fLastTrigger = fTarget;
				break;
#endif
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
				Object* object = core->GetObject(actor, FindObjectNode(trig));
				if (object != NULL)
					returnValue = core->See(actor, object);
					// || core->Hear(fTarget, object);
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
				/*object_node* object = FindObjectNode(trig);
				if (object != NULL) {
					Actor* objectOverRegion = core->GetObject(
							dynamic_cast<Region*>(actor));
					if (objectOverRegion != NULL)
						returnValue = objectOverRegion->MatchNode(object);
				}*/

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

			case 0x4c:
			{
#if 0
				// Entered(O:Object)
				object_node* node = FindObjectNode(trig);
				Region* region = dynamic_cast<Region*>(fTarget.Target());
				//std::vector<std::string>::const_iterator i;

				Object* object = Object::GetMatchingObjectFromList(
										region->
										LastScriptRoundResults()->
										EnteredActors(), node);
				if (object != NULL) {
					fLastTrigger = object;
					returnValue = true;
				}
#endif
				break;
			}
			default:
			{
				printf("SCRIPT: UNIMPLEMENTED TRIGGER!!!\n");
				printf("SCRIPT: %s (%d 0x%x)\n", IDTable::TriggerAt(trig->id).c_str(),
								trig->id, trig->id);
				trig->Print();
				break;
			}
		}
	} catch (...) {
		printf("SCRIPT: EvaluateTrigger() caught exception");
	}
	if (trig->flags != 0)
		returnValue = !returnValue;
#if DEBUG_SCRIPTS
	printf("SCRIPT: (%s)\n", returnValue ? "TRUE" : "FALSE");
#endif
	return returnValue;
}


bool
Script::_ExecuteActions(node* responseSet)
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

	// TODO: Fix this and take the probability into account
	int randomResponse = rand() % i;
	action_node* action = FindActionNode(responses[randomResponse]);
	// More than one action
	while (action != NULL) {
		if (!_ExecuteAction(action))
			return false;
		action = static_cast<action_node*>(action->Next());
	}
	return true;
}


bool
Script::_ExecuteAction(action_node* act)
{
#if DEBUG_SCRIPTS
	printf("SCRIPT: %s (%d 0x%x)\n", IDTable::ActionAt(act->id).c_str(), act->id, act->id);
	act->Print();
#endif
	Core* core = Core::Get();
	Actor* thisActor = dynamic_cast<Actor*>(fTarget.Target());

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

			Actor* actor = new Actor(act->string1, point, act->integer1);
			Core::Get()->AddActorToCurrentArea(actor);
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
			/*Actor* actor = dynamic_cast<Actor*>(fTarget.Target());
			if (actor != NULL) {
				WalkTo* walkTo = new WalkTo(actor, act->where);
				actor->AddAction(walkTo);
				actor->StopCheckingConditions();
			}*/
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
			// TODO: Implement
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
			if (fTarget.Target() == NULL)
				printf("NULL TARGET\n");
			std::ostringstream stringStream;
			stringStream << fTarget.Target()->Name() << " " << act->integer1;

			GameTimer::Add(stringStream.str().c_str(), act->integer2);

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
			if (thisActor != NULL)
				thisActor->SetInterruptable(act->integer1 == 1);
			break;
		}
		case 0x1E:
		{
			std::string variableScope;
			variableScope.append(act->string1, 6);
			std::string variableName;
			variableName.append(&act->string1[6]);

			if (variableScope.compare("LOCALS") == 0) {
				if (fTarget != NULL)
					fTarget.Target()->SetVariable(variableName.c_str(),
							act->integer1);
			} else {
				// TODO: Check for AREA variables
				core->SetVariable(variableName.c_str(),
						act->integer1);
			}
			break;
		}
		case 0x53:
		{
			/* 83 SmallWait(I:Time*) */
			// TODO: The time is probably wrong
			//
			/*Wait* wait = new Wait(thisActor, act->integer1);
			thisActor->AddAction(wait);*/
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
		case 111:
		{
			/* DESTROYSELF() (111 0x6f) */
			// TODO: Delete it for real
			fTarget.Target()->SetStale(true);
			return false;
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
		case 207:
		{
			/* 207 MoveToPointNoInterrupt(P:Point*)
			 * This action causes the active creature to move to the specified coordinates.
			 * The action will update the position of creatures as stored in ARE files
			 * (first by setting the coordinates of the destination point, then by setting
			 * the coordinates of the current point once the destination is reached).
			 * Conditions are not checked until the destination point is reached.*/
			Actor* actor = dynamic_cast<Actor*>(fTarget.Target());
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
			break;
		}
		case 286: // 0x11e
		{
			/* HIDEGUI */
			core->CurrentRoom()->HideGUI();
			break;
		}
		default:
			printf("SCRIPT: UNIMPLEMENTED ACTION!!!\n");
			printf("SCRIPT: %s (%d 0x%x)\n", IDTable::ActionAt(act->id).c_str(), act->id, act->id);
			act->Print();
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
	std::cout << "Object:" << std::endl;
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
