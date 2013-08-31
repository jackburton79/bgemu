#include "Actor.h"
#include "Core.h"
#include "CreResource.h"
#include "Door.h"
#include "IDSResource.h"
#include "Parsing.h"
#include "ResManager.h"
#include "Room.h"
#include "Script.h"
#include "Timer.h"


#include <algorithm>

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
	fTarget(NULL),
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


Object*
Script::FindObject(node* start) const
{
	object_node* objectNode = FindObjectNode(start);
	while (objectNode != NULL && Object::IsDummy(objectNode)) {
		objectNode = static_cast<object_node*>(objectNode->Next());
	}

	if (objectNode == NULL)
		return NULL;

	//objectNode->Print();
	return Core::Get()->GetObject(fTarget, objectNode);
}


bool
Script::Execute()
{
	// TODO: Move this to a better place
	if (fTarget != NULL)
		fTarget->NewScriptRound();

	// for each CR block
	// for each CO block
	// check triggers
	// if any trigger
	// find response_set block
	// choose response
	// execute actions

	::node* nextScript = fRootNode;
	while (nextScript != NULL) {
		printf("*** SCRIPT START ***\n");
		::node* condRes = FindNode(BLOCK_CONDITION_RESPONSE, nextScript);
		do {
			::node* condition = FindNode(BLOCK_CONDITION, condRes);
			while (condition != NULL) {
				if (!_CheckTriggers(condition))
					break;

				::node* responseSet = condition->Next();
				if (responseSet == NULL)
					break;

				_ExecuteActions(responseSet);

				condition = responseSet->Next();
			}
			condRes = condRes->Next();
		} while (condRes != NULL);

		printf("*** SCRIPT END ***\n");
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
	//fOrTriggers = 0;

	trigger_node* trig = FindTriggerNode(conditionNode);
	bool evaluation = true;
	while (trig != NULL) {
		/*if (trig->id == 0x4089) { // OR
			_EvaluateTrigger(trig);
			// we don't care about the return value of this one
		} else if (fOrTriggers > 0) {
			evaluation = _EvaluateTrigger(trig);
			if (evaluation)
				break;
			fOrTriggers--;
		} else {*/
			evaluation = _EvaluateTrigger(trig) && evaluation;
			if (!evaluation)
				break;
		//}
		trig = static_cast<trigger_node*>(trig->Next());
	}
	return evaluation;
}


bool
Script::_EvaluateTrigger(trigger_node* trig)
{
	Actor* actor = dynamic_cast<Actor*>(fTarget);
	if (actor != NULL && actor->SkipConditions())
		return false;

	std::cout << "*** " << fTarget->Name() << " ***" << std::endl;

	printf("%s (%d 0x%x)\n", IDTable::TriggerAt(trig->id).c_str(),
				trig->id, trig->id);
	trig->Print();

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
				object_node* object = FindObjectNode(trig);
				returnValue = Object::CheckIfNodeInList(object,
						fTarget->LastScriptRoundResults()->Attackers());
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
				object_node* object = FindObjectNode(trig);
				returnValue = Object::CheckIfNodeInList(object,
						fTarget->LastScriptRoundResults()->Hitters());
				break;
			}
			case 0x0022:
			{
				/* TimerExpired(I:ID*) */
				Timer* timer = Timer::Get(trig->string1);
				if (timer != NULL && timer->Expired())
					returnValue = true;
				break;
			}
			case 0x0027:
			{
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
				Object* object = core->GetObject(fTarget, FindObjectNode(trig));
				if (object != NULL && core->Distance(fTarget, object) <= 30
						&& object->LastScriptRoundResults()->Shouted()
						== trig->parameter1) {
					returnValue = true;
				}
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
			case 0x0070:
			{
				/* 0x0070 Clicked(O:Object*)
				 *	Only for trigger regions.
				 *	Returns true if the specified object
				 *	clicked on the trigger region running this script.
				 */
				// TODO: Implement.
				object_node* objectNode = FindObjectNode(trig);
				objectNode->Print();
				returnValue = false;
				break;
			}
			case 0x400C:
			{
				/*0x400C Class(O:Object*,I:Class*Class)*/
				Object* object = core->GetObject(fTarget, FindObjectNode(trig));
				if (object != NULL)
					returnValue = object->IsClass(trig->parameter1);
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
					Actor* actor = dynamic_cast<Actor*>(fTarget);
					if (actor != NULL)
						variableValue = actor->GetVariable(variableName.c_str());
				} else {
					// TODO: Check for AREA variables, currently we
					// treat AREA variables as global variables
					variableValue = Core::Get()->GetVariable(variableName.c_str());
				}
				returnValue = variableValue == trig->parameter1;
				break;
			}
			case 0x4017:
			{
				// Race()
				Object* object = core->GetObject(
						fTarget, FindObjectNode(trig));
				if (object != NULL)
					returnValue = object->IsRace(trig->parameter1);

				break;
			}

			case 0x4018:
			{
				/* 0x4018 Range(O:Object*,I:Range*)
				Returns true only if the specified object
				is within distance given (in feet) of the active CRE. */
				Object* object = core->GetObject(
						fTarget, FindObjectNode(trig));
				returnValue = core->Distance(object, fTarget) <= trig->parameter1;
				break;
			}

			case 0x401C:
			{
				/* See(O:Object*)
				 * Returns true only if the active CRE can see
				 * the specified object which must not be hidden or invisible.
				 */
				Object* object = core->GetObject(fTarget, FindObjectNode(trig));
				if (object != NULL)
					returnValue = core->See(fTarget, object);
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

			case 0x4040:
			{
				/* GlobalTimerExpired(S:Name*,S:Area*) (16448 0x4040) */
				// TODO: Merge Name and Area before passing them to the
				// Timer::Get() method (also below)
				Timer* timer = Timer::Get(trig->string1);
				returnValue = timer != NULL && timer->Expired();
				break;
			}
			case 0x4041:
			{
				/* GlobalTimerNotExpired(S:Name*,S:Area*) */
				Timer* timer = Timer::Get(trig->string1);
				returnValue = timer == NULL || !timer->Expired();
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
			case 0x4068:
			{
				/* TimeGT(I:Time*Time)
				 * Returns true only if the current time is greater than
				 * that specified. Hours are offset by 30 minutes,
				 * e.g. TimeGT(1) is true between 01:30 and 02:29.
				 */
				break;
			}

			case 0x4072:
			{
				/* NumDeadGT(S:Name*,I:Num*) (0x4072)*/
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
				Object* object = core->GetObject(fTarget, FindObjectNode(trig));
				if (object != NULL)
					returnValue = core->See(fTarget, object);
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
				Door *door = Door::GetByName(doorObj->name);
				if (door != NULL) {
					bool paramOpen = trig->parameter1 == 1;
					returnValue = door->Opened() == paramOpen;
				}
				break;
			}

			case 0x407E:
			{
				/* 0x407E AreaCheck(S:ResRef*)
				 * Returns true only if the active CRE
				 * is in the area specified.
				 */
				returnValue = !strcmp(Room::Get()->Name(),
						trig->string1);
				break;
			}
			case 0x4089:
			{
				/*0x4089 OR(I:OrCount*)*/
				//fOrTriggers = trig->parameter1;
				break;
			}
			case 0x4c:
			{
				// Entered(O:Object)
				object_node* obj = FindObjectNode(trig);
				obj->Print();
				break;
			}
			default:
			{
				printf("UNIMPLEMENTED TRIGGER!!!\n");

				break;
			}
		}
	} catch (...) {
		printf("EvaluateTrigger() caught exception");
	}
	if (trig->flags != 0)
		returnValue = !returnValue;

	printf("\t*** %s (flags: %d) ***\n", returnValue ? "TRUE" : "FALSE", trig->flags);
	return returnValue;
}


void
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
		_ExecuteAction(action);
		action = static_cast<action_node*>(action->Next());
	}
}


void
Script::_ExecuteAction(action_node* act)
{
	printf("%s (%d 0x%x)\n", IDTable::ActionAt(act->id).c_str(), act->id, act->id);
	act->Print();
	Core* core = Core::Get();
	Actor* thisActor = dynamic_cast<Actor*>(fTarget);

	switch (act->id) {
		case 0x03:
		{
			/* Attack(O:Target*) */
			Object* targetObject = FindObject(act);
			if (targetObject != NULL) {
				Actor* targetActor = dynamic_cast<Actor*>(targetObject);
				if (thisActor != NULL && targetActor != NULL) {
					std::cout << thisActor->Name() << " attacks ";
					std::cout << targetActor->Name() << std::endl;
					thisActor->Attack(targetActor);
				}
			}
			break;
		}
		case 0x07:
		{
			/* CreateCreature(S:NewObject*,P:Location*,I:Face*) */
			Actor* actor = new Actor(act->string1, act->where, act->parameter);
			Actor::Add(actor);
			break;
		}

		case 29:
		{
			/* RunAwayFrom(O:Creature*,I:Time*) */
			/*object_node* objBlock = FindObjectNode(act);

			if (objBlock == NULL)
				break;
*/
			Object* targetObject = FindObject(act);
			//Object* targetObject = core->GetObject(fTarget, objBlock);
			if (targetObject != NULL && thisActor != NULL) {
				std::cout << thisActor->Name();
				std::cout << " run away from " << targetObject->Name() << std::endl;
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
			Timer::Add(act->string1, act->parameter);

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
			if (thisActor != NULL)
				thisActor->SetInterruptable(act->parameter == 1);
			break;
		}
		case 0x1E:
		{
			std::string variableScope;
			variableScope.append(act->string1, 6);
			std::string variableName;
			variableName.append(&act->string1[6]);

			if (variableScope.compare("LOCALS") == 0) {
				if (thisActor != NULL)
					thisActor->SetVariable(variableName.c_str(),
							act->parameter);
			} else {
				// TODO: Check for AREA variables
				core->SetVariable(variableName.c_str(),
						act->parameter);
			}
			break;
		}
		case 0x53:
		{
			/* 83 SmallWait(I:Time*) */
			// TODO: Implement
			break;
		}

		case 106:
		{
			/* Shout */
			// Check if target is silenced
			if (thisActor != NULL)
				thisActor->Shout(act->parameter);
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
			if (thisActor != NULL && thisActor->IsInterruptable())
				core->FlyToPoint(thisActor, act->where, act->parameter);
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
			Object* targetObject = FindObject(act);
			if (targetObject != NULL) {
				Actor* targetActor = dynamic_cast<Actor*>(targetObject);
				if (thisActor != NULL && targetActor != NULL) {
					std::cout << thisActor->Name() << " attacks ";
					std::cout << targetActor->Name() << std::endl;
					thisActor->Attack(targetActor);
				}
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
			Actor* actor = dynamic_cast<Actor*>(fTarget);
			if (actor != NULL) {
				actor->SetDestination(act->where);
				actor->StopCheckingConditions();
			}
			break;
		}
		case 228: // 0xe4
		{
			/* CreateCreatureImpassable(S:NewObject*,P:Location*,I:Face*) (228 0xe4) */
			/* This action creates the specified creature
			 * on a normally impassable surface (e.g. on a wall,
			 * on water, on a roof). */
			Actor* actor = new Actor(act->string1, act->where, act->parameter);
			std::cout << "Created actor " << act->string1 << " on ";
			std::cout << act->where.x << ", " << act->where.y << std::endl;
			actor->SetDestination(act->where);
			Actor::Add(actor);
			break;
		}
		default:
			printf("UNIMPLEMENTED ACTION!!!\n");

			break;
	}
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
	std::cout << std::endl;
	std::cout << "ea: " << IDTable::EnemyAllyAt(ea) << " (" << ea << "), ";
	std::cout << "general: " << IDTable::GeneralAt(general) << " (" << general << "), ";
	std::cout << "race: " << IDTable::RaceAt(race) << " (" << race << "), ";
	std::cout << "class: " << IDTable::ClassAt(classs) << " (" << classs << "), ";
	std::cout << std::endl;
	std::cout << "specific: " << IDTable::SpecificAt(specific) << " (" << specific << "), ";
	std::cout << "gender: " << IDTable::GenderAt(gender) << " (" << gender << "), ";
	std::cout << "alignment: " << IDTable::AlignmentAt(alignment) << " (" << alignment << "), " << std::endl;
	std::cout << "identifiers: ";
	for (int32 i = 0; i < 5; i++) {
		 std::cout << IDTable::ObjectAt(identifiers[i]) << " ";
	}
	std::cout << std::endl;
	if (Core::Get()->Game() == GAME_TORMENT)
		std::cout << "point: " << point.x << ", " << point.y << std::endl;
	if (name[0] != '\0')
		 std::cout << "name: " << name << std::endl;
}


// action
action_node::action_node()
{
}


void
action_node::Print() const
{
	printf("id: %d, parameter: %d, point: (%d, %d), %d, %d, %s, %s\n",
			id, parameter, where.x, where.y, e, f, string1, string2);
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
