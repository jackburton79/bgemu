/*
 * ScriptContext.cpp
 *
 *  Created on: 12/lug/2012
 *      Author: stefano
 */

#include "Actor.h"
#include "Core.h"
#include "Door.h"
#include "IDSResource.h"
#include "Script.h"
#include "Scriptable.h"
#include "ScriptContext.h"

#include <stdlib.h>

ScriptContext::ScriptContext(Scriptable* target, Script* script)
	:
	fTarget(target),
	fScript(script),
	fOrTriggers(-1)
{
}


ScriptContext::~ScriptContext()
{

}


void
ScriptContext::ExecuteScript()
{
	fTarget->NewScriptRound();

	// for each CR block
	// for each CO block
	// check triggers
	// if any trigger
	// find response_set block
	// choose response
	// execute actions
	//printf("*** SCRIPT START ***\n");
	::node* root = fScript->RootNode();
	::node* condRes = fScript->FindNode(BLOCK_CONDITION_RESPONSE, root);
	do {
		::node* cond = fScript->FindNode(BLOCK_CONDITION, condRes);
		while (cond != NULL) {
			if (!_CheckTriggers(cond))
				break;

			::node* responseSet = cond->Next();
			if (responseSet == NULL)
				break;

			_ExecuteActions(responseSet);

			cond = responseSet->Next();
		}
		condRes = condRes->Next();
	} while (condRes != NULL);

	//printf("*** SCRIPT END ***\n");

	fScript->SetProcessed();
}


bool
ScriptContext::_CheckTriggers(node* conditionNode)
{
	fOrTriggers = 0;

	trigger* trig = static_cast<trigger*>(
			fScript->FindNode(BLOCK_TRIGGER, conditionNode));
	bool evaluation = true;
	while (trig != NULL) {
		if (trig->id == 0x4089) { // OR
			_EvaluateTrigger(trig);
			// we don't care about the return value of this one
		} else if (fOrTriggers > 0) {
			evaluation = _EvaluateTrigger(trig);
			if (evaluation)
				break;
			fOrTriggers--;
		} else {
			evaluation = _EvaluateTrigger(trig) && evaluation;
			if (!evaluation)
				break;
		}
		trig = static_cast<trigger*>(trig->Next());
	}
	return evaluation;
}


bool
ScriptContext::_EvaluateTrigger(trigger* trig)
{
	// TODO: Move this method to Script ?
	printf("%s (0x%x)\n", TriggerIDS()->ValueFor(trig->id), trig->id);
	trig->Print();

	Actor* actor = dynamic_cast<Actor*>(fTarget);
	if (actor != NULL && actor->SkipConditions())
		return false;

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
				object* obj = static_cast<object*>(trig->FindNode(BLOCK_OBJECT));
				obj->Print();
				returnValue = false;
				break;
			}
			case 0x0027:
			{
				break;
			}
			case 0x0036:
			{
				/*0x0036 OnCreation()
				Returns true if the script is processed for the first time this session,
				e.g. when a creature is created (for CRE scripts) or when the player
				enters an area (for ARE scripts).*/
				returnValue = !fScript->Processed();
				break;
			}
			case 0x400F:
			{
				/*0x400F Global(S:Name*,S:Area*,I:Value*)
				Returns true only if the variable with name 1st parameter
				of type 2nd parameter has value 3rd parameter.*/
				returnValue = Core::Get()->GetVariable(trig->string1) == trig->parameter1;
				break;
			}
			case 0x401C:
			{
				/* See(O:Object*)
				 * Returns true only if the active CRE can see
				 * the specified object which must not be hidden or invisible.
				 */
				object* obj = static_cast<object*>(trig->FindNode(BLOCK_OBJECT));
				obj->Print();
				printf("See %s ?\n", obj->name);
				returnValue = false;
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
				returnValue = Core::Get()->GetVariable(trig->string1) > trig->parameter1;
				break;
			}
			case 0x4035:
			{	/*
				0x4035 GlobalLT(S:Name*,S:Area*,I:Value*)
				As above except for less than. */
				returnValue = Core::Get()->GetVariable(trig->string1) < trig->parameter1;
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
			case 0x4072:
			{
				/* NumDeadGT(S:Name*,I:Num*) (0x4072)*/
				break;
			}
			case 0x4076:
			{
				/*
				 * 0x4076 OpenState(O:Object*,I:Open*BOOLEAN)
				 * NT Returns true only if the open state of the specified door
				 * matches the state specified in the 2nd parameter.
				 */
				object* doorObj = static_cast<object*>(trig->FindNode(BLOCK_OBJECT));
				Door *door = Door::GetByName(doorObj->name);
				if (door != NULL) {
					bool paramOpen = trig->parameter1 == 1;
					returnValue = door->Opened() == paramOpen;
				}
				break;
			}

			case 0x4089:
			{
				/*0x4089 OR(I:OrCount*)*/
				fOrTriggers = trig->parameter1;
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
	return returnValue;
}


void
ScriptContext::_ExecuteActions(node* responseSet)
{
	response* responses[5];
	response* resp = static_cast<response*>(
			fScript->FindNode(BLOCK_RESPONSE, responseSet));
	int i = 0;
	int totalChance = 0;
	while (resp != NULL) {
		responses[i] = resp;
		totalChance += responses[i]->probability;
		resp = static_cast<response*>(resp->Next());
		i++;
	}

	// TODO: Fix this and take the probability into account
	int randomResponse = rand() % i;
	action* act = static_cast<action*>(
		fScript->FindNode(BLOCK_ACTION, responses[randomResponse]));
	while (act != NULL) {
		_ExecuteAction(act);
		act = static_cast<action*>(act->Next());
	}
}


void
ScriptContext::_ExecuteAction(action* act)
{
	printf("%s (0x%x)\n", ActionIDS()->ValueFor(act->id), act->id);
	act->Print();
	switch (act->id) {
		case 0x07:
		{
			/* CreateCreature(S:NewObject*,P:Location*,I:Face*) */
			Actor* actor = new Actor(act->string1, act->where, act->parameter);
			Actor::Add(actor);
			break;
		}
		case 0x24:
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
		case 85:
		{
			/* 100 RandomWalk */
			Actor* actor = dynamic_cast<Actor*>(fTarget);
			if (actor != NULL)
				Core::Get()->RandomWalk(actor);
			break;
		}
		case 0x1E:
		{
			Core::Get()->SetVariable(act->string1, act->parameter);
			break;
		}
		case 0x53:
		{
			/* 83 SmallWait(I:Time*) */
			// TODO: Implement
			break;
		}

		case 0x64:
		{
			/* 100 RandomFly */
			Actor* actor = dynamic_cast<Actor*>(fTarget);
			if (actor != NULL)
				Core::Get()->RandomFly(actor);

			break;
		}
		case 0x65:
		{
			/* 101 FlyToPoint(Point, Time) */
			Actor* actor = dynamic_cast<Actor*>(fTarget);
			if (actor != NULL)
				Core::Get()->FlyToPoint(actor, act->where, act->parameter);
			break;
		}
		case 0xA7:
		{
			//printf("STARTMOVIE: %s\n", act->string1);
			Core::Get()->PlayMovie(act->string1);

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
		default:
			printf("UNIMPLEMENTED ACTION!!!\n");

			break;
	}
}
