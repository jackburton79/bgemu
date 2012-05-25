#include "World.h"

#include "Actor.h"
#include "IDSResource.h"
#include "ResManager.h"
#include "Room.h"
#include "Script.h"
#include "TLKResource.h"

static TLKResource *sDialogs;
static IDSResource *sGeneral;
static IDSResource *sAnimate;
static IDSResource *sRaces;
static IDSResource *sGenders;
static IDSResource *sClasses;
static IDSResource *sSpecifics;
static IDSResource *sTriggers;
static IDSResource *sActions;

static World *sWorld = NULL;

World::World()
	:
	fCurrentRoom(NULL),
	fScript(NULL)
{
	sWorld = this;

	sDialogs = gResManager->GetTLK(kDialogResource);
	sAnimate = gResManager->GetIDS("ANIMATE");
	sGeneral = gResManager->GetIDS("GENERAL");
	sRaces = gResManager->GetIDS("RACE");
	sGenders = gResManager->GetIDS("GENDER");
	sClasses = gResManager->GetIDS("CLASS");
	sSpecifics = gResManager->GetIDS("SPECIFIC");
	sTriggers = gResManager->GetIDS("TRIGGER");
	sActions = gResManager->GetIDS("ACTION");
}


World::~World()
{
	gResManager->ReleaseResource(sDialogs);
	gResManager->ReleaseResource(sSpecifics);
	gResManager->ReleaseResource(sGenders);
	gResManager->ReleaseResource(sRaces);
	gResManager->ReleaseResource(sClasses);
	gResManager->ReleaseResource(sGeneral);
	gResManager->ReleaseResource(sAnimate);
	delete fCurrentRoom;
}


void
World::EnterArea(const char *name)
{
	fCurrentRoom = new Room();
	if (!fCurrentRoom->Load(name))
		throw false;

	// The area script
	if (fScript != NULL)
		_ExecuteScript(fScript);
}


Room *
World::CurrentArea() const
{
	return fCurrentRoom;
}


void
World::SetVariable(const char* name, int32 value)
{
	fVariables[name] = value;
}


int32
World::GetVariable(const char* name)
{
	//printf("%s %d\n", name, fVariables[name]);
	return fVariables[name];
}


void
World::AddScript(Script* script)
{
	fScript = script;
	//fScript->Print();
}


void
World::CheckScripts()
{
	//if (fScript != NULL)
		//_ExecuteScript(fScript);
}


void
World::_ExecuteScript(Script *script)
{
	// for each CR block
	// for each CO block
	// check triggers
	// if any trigger
	// find response_set block
	// choose response
	// execute actions
	printf("*** SCRIPT START ***\n");
	::node* root = script->RootNode();
	::node* condRes = fScript->FindNode(BLOCK_CONDITION_RESPONSE, root);
	do {
		::node* cond = fScript->FindNode(BLOCK_CONDITION, condRes);
		while (cond != NULL) {
			// If a trigger was false, don't consider responses
			if (_CheckTriggers(cond))
				break;

			::node* responseSet = cond->Next();
			if (responseSet == NULL)
				break;

			_ExecuteActions(responseSet);

			cond = responseSet->Next();
		}
		condRes = condRes->Next();
	} while (condRes != NULL);

	script->SetProcessed();
}


bool
World::_CheckTriggers(node* conditionNode)
{
	trigger* trig = static_cast<trigger*>(
		fScript->FindNode(BLOCK_TRIGGER, conditionNode));
	bool evaluation = true;
	while (trig != NULL) {
		evaluation = _EvaluateTrigger(trig) && evaluation;
		if (!evaluation)
			break;
		trig = static_cast<trigger*>(trig->Next());
	}
	return evaluation;
}


bool
World::_EvaluateTrigger(trigger* trig)
{
	// TODO: Move this method to Script ?
	trig->Print();
	printf("%s (0x%x)\n", TriggerIDS()->ValueFor(trig->id), trig->id);

	try {
		switch (trig->id) {
			case 0x0036:
			{
				/*0x0036 OnCreation()
				Returns true if the script is processed for the first time this session,
				e.g. when a creature is created (for CRE scripts) or when the player
				enters an area (for ARE scripts).*/
				return !fScript->Processed();
			}
			case 0x400F:
			{
				/*0x400F Global(S:Name*,S:Area*,I:Value*)
				Returns true only if the variable with name 1st parameter
				of type 2nd parameter has value 3rd parameter.*/
				return GetVariable(trig->string1) == trig->parameter1;
			}
			case 0x4034:
			{
				/*0x4034 GlobalGT(S:Name*,S:Area*,I:Value*)
				See Global(S:Name*,S:Area*,I:Value*) except the variable
				must be greater than the value specified to be true.
				*/
				return GetVariable(trig->string1) > trig->parameter1;
			}
			case 0x4035:
			{	/*
				0x4035 GlobalLT(S:Name*,S:Area*,I:Value*)
				As above except for less than. */
				return GetVariable(trig->string1) < trig->parameter1;
			}
#if 0
			case 0x4089:
			{
				/*0x4089 OR(I:OrCount*)
				Returns true if any of the next 'OrCount' triggers returns true.*/
				break;
			}
#endif
			default:
			{
				printf("UNIMPLEMENTED TRIGGER!!!\n");
				break;
			}
		}
	} catch (...) {
		printf("EvaluateTrigger() caught exception");
	}
	return false;
}


void
World::_ExecuteActions(node* responseSet)
{
	action* act = static_cast<action*>(
		fScript->FindNode(BLOCK_ACTION, responseSet));
	while (act != NULL) {
		_ExecuteAction(act);
		act = static_cast<action*>(act->Next());
	}
}


void
World::_ExecuteAction(action* act)
{
	act->Print();
	printf("%s (0x%x)\n", ActionIDS()->ValueFor(act->id), act->id);

	switch (act->id) {
		case 0x07:
		{
			/* CreateCreature(S:NewObject*,P:Location*,I:Face*) */

			fCurrentRoom->CreateCreature(act->string1, act->where, act->parameter);
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
		case 0x1E:
			SetVariable(act->string1, act->parameter);
			break;
		case 0x53:
		{
			/* 83 SmallWait(I:Time*) */
			// TODO: Implement
			break;
		}
		default:
			break;
	}
}


// global functions
World*
GetWorld()
{
	return sWorld;
}


TLKResource*
Dialogs()
{
	return sDialogs;
}


IDSResource*
GeneralIDS()
{
	return sGeneral;
}


IDSResource*
AnimateIDS()
{
	return sAnimate;
}


IDSResource*
RacesIDS()
{
	return sRaces;
}


IDSResource*
GendersIDS()
{
	return sGenders;
}


IDSResource*
ClassesIDS()
{
	return sClasses;
}


IDSResource*
SpecificIDS()
{
	return sSpecifics;
}


IDSResource*
TriggerIDS()
{
	return sTriggers;
}


IDSResource*
ActionIDS()
{
	return sActions;
}
