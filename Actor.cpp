#include "Actor.h"

#include "Action.h"
#include "Animation.h"
#include "AnimationFactory.h"
#include "BamResource.h"
#include "BCSResource.h"
#include "Bitmap.h"
#include "Container.h"
#include "Core.h"
#include "CreResource.h"
#include "DLGResource.h"
#include "Door.h"
#include "GraphicsEngine.h"
#include "IDSResource.h"
#include "ITMResource.h"
#include "PathFind.h"
#include "Polygon.h"
#include "RectUtils.h"
#include "ResManager.h"
#include "Room.h"
#include "Script.h"
#include "WedResource.h"

#include <algorithm>
#include <assert.h>
#include <string>


Actor::Actor(IE::actor &actor)
	:
	Object(actor.name),
	fActor(&actor),
	fAnimationFactory(NULL),
	fCRE(NULL),
	fOwnsActor(false),
	fDontCheckConditions(false),
	fIsInterruptable(true),
	fFlying(false),
	fSelected(false),
	fAction(ACT_STANDING),
	fPath(NULL),
	fSpeed(2)
{
	_Init();
}


Actor::Actor(IE::actor &actor, CREResource* cre)
	:
	Object(actor.name),
	fActor(&actor),
	fAnimationFactory(NULL),
	fCRE(cre),
	fOwnsActor(false),
	fDontCheckConditions(false),
	fIsInterruptable(true),
	fFlying(false),
	fSelected(false),
	fAction(ACT_STANDING),
	fPath(NULL),
	fSpeed(2)
{
	_Init();
}


Actor::Actor(const char* creName, IE::point position, int face)
	:
	Object(creName),
	fActor(NULL),
	fAnimationFactory(NULL),
	fCRE(NULL),
	fOwnsActor(true),
	fDontCheckConditions(false),
	fFlying(false),
	fSelected(false),
	fAction(ACT_STANDING),
	fPath(NULL),
	fSpeed(2)
{
	fActor = new IE::actor;
	fActor->cre = creName;
	memcpy(fActor->name, fActor->cre.name, 8);
	fActor->name[8] = 0;
	fActor->orientation = face;
	//fActor->orientation = 0;
	fActor->position = position;

	_Init();
}


void
Actor::_Init()
{
	fSelected = false;
	fCurrentAnimation = NULL;

	if (fCRE == NULL)
		fCRE = gResManager->GetCRE(fActor->cre);

	// TODO: Get all scripts ? or just the specific one ?

	if (fCRE == NULL)
		throw "error!!!";

	Core::Get()->RegisterObject(this);

	std::string baseName = IDTable::AniSndAt(fCRE->AnimationID());
	fAnimationFactory = AnimationFactory::GetFactory(baseName.c_str());
	std::cout << "Animation: " << baseName << " (" << std::hex;
	std::cout << fCRE->AnimationID() << ")" << std::endl;

	/*std::cout << "colors:" << std::endl << std::dec;
	std::cout << "\tmajor:" << (int)fCRE->Colors().major << std::endl;
	std::cout << "\tminor:" << (int)fCRE->Colors().minor << std::endl;
	std::cout << "\tarmor:" << (int)fCRE->Colors().armor << std::endl;
	std::cout << "\thair:" << (int)fCRE->Colors().hair << std::endl;*/
	std::cout << std::dec;
	std::cout << Name() << " enum: local: " << fCRE->LocalActorEnum();
	std::cout << ", global: " << fCRE->GlobalActorEnum() << std::endl;

	// TODO: Are we overwriting the actor specific stuff here ?
	fActor->script_override = fCRE->OverrideScriptName();
	fActor->script_class = fCRE->ClassScriptName();
	fActor->script_race = fCRE->RaceScriptName();
	fActor->script_default = fCRE->DefaultScriptName();
	fActor->script_general = fCRE->GeneralScriptName();

	Script* script = MergeScripts();
	SetScript(script);

	fActor->Print();
	/*for (uint32 i = 0; i < kNumItemSlots; i++) {
		IE::item* item = fCRE->ItemAtSlot(i);
		if (item != NULL) {
			item->Print();
			ITMResource* itemRes = gResManager->GetITM(item->name);
			if (itemRes != NULL) {
				std::cout << "type: " << std::dec << itemRes->Type() << std::endl;
			}
			gResManager->ReleaseResource(itemRes);

		}
	}*/

	//TODO: some orientations are bad. Why?!?!?!
	if (fActor->orientation > IE::ORIENTATION_SE) {
		std::cerr << "Weird orientation " << fActor->orientation << std::endl;
		fActor->orientation = 0;
	}

	fPath = new PathFinder(PathFinder::kStep, Room::IsPointPassable);
}


Actor::~Actor()
{
	if (fOwnsActor)
		delete fActor;

	gResManager->ReleaseResource(fCRE);

	if (fAnimationFactory != NULL) {
		AnimationFactory::ReleaseFactory(fAnimationFactory);
		fAnimationFactory = NULL;
	}

	delete fPath;
}


const ::Bitmap*
Actor::Bitmap() const
{
	if (fCurrentAnimation == NULL)
		throw "Actor::Bitmap(): No current animation!";

	return fCurrentAnimation->Bitmap();
}


// Returns the rect containing the current actor image
GFX::rect
Actor::Frame() const
{
	const GFX::rect& frame = Bitmap()->Frame();
	IE::point leftTop = offset_point(Position(),
								-(frame.x + frame.w / 2),
								-(frame.y + frame.h / 2));

	return GFX::rect(leftTop.x, leftTop.y, frame.w, frame.h);
}


IE::point
Actor::Position() const
{
	return fActor->position;
}


void
Actor::SetPosition(const IE::point& position)
{
	fActor->position = position;
}


IE::orientation
Actor::Orientation() const
{
	return (IE::orientation)fActor->orientation;
}


void
Actor::SetOrientation(IE::orientation o)
{
	if (o < 0 || o > IE::ORIENTATION_SE)
		o = IE::orientation(0);
	fActor->orientation = o;
}


IE::point
Actor::Destination() const
{
	return fActor->destination;
}


void
Actor::SetDestination(const IE::point& point)
{
	fActor->destination = fPath->SetPoints(fActor->position, point);
}


uint32
Actor::NumTimesTalkedTo() const
{
	return fActor->num_times_talked_to;
}


void
Actor::InitiateDialogWith(Actor* actor)
{
	std::cout << "InitiateDialogWith " << fActor->dialog << std::endl;
	DLGResource* dlgResource = gResManager->GetDLG(fActor->dialog);

	gResManager->ReleaseResource(dlgResource);
}


const char*
Actor::Area() const
{
	return fArea.c_str();
}


void
Actor::SetArea(const char* areaName)
{
	fArea = areaName;
}


/* virtual */
IE::point
Actor::NearestPoint(const IE::point& point) const
{
	IE::point newPoint = Position();

	newPoint.x += fActor->movement_restriction_distance;
	return newPoint;
}


/* virtual */
void
Actor::ClickedOn(Object* target)
{
	if (target == NULL)
		return;

	Object::ClickedOn(target);

	// TODO: Add a "mode" to the ClickedOn method, to distinguish
	// an attack from a dialog start, etc

	if (Door* door = dynamic_cast<Door*>(target)) {
		IE::point point = door->NearestPoint(Position());
		WalkTo* walkToAction = new WalkTo(this, point);
		AddAction(walkToAction);
		Toggle* toggleAction = new Toggle(this, door);
		AddAction(toggleAction);
	} else if (Actor* actor = dynamic_cast<Actor*>(target)) {
		Attack* attackAction = new Attack(this, actor);
		AddAction(attackAction);
	} else if (Container* container = dynamic_cast<Container*>(target)) {
		WalkTo* walkTo = new WalkTo(this, container->NearestPoint(Position()));
		AddAction(walkTo);
	}
}


void
Actor::Shout(int number)
{
	CurrentScriptRoundResults()->fShouted = number;
}


void
Actor::SetFlying(bool fly)
{
	fFlying = fly;
}


bool
Actor::IsFlying() const
{
	return fFlying;
}


void
Actor::SetInterruptable(bool interrupt)
{
	fIsInterruptable = interrupt;
}


bool
Actor::IsInterruptable() const
{
	return fIsInterruptable;
}


void
Actor::Select(bool select)
{
	fSelected = select;
}


bool
Actor::IsSelected() const
{
	return fSelected;
}


CREResource *
Actor::CRE() const
{
	return fCRE;
}


static bool
IsValid(const res_ref& scriptName)
{
	res_ref noneRef = "None";
	if (scriptName.name[0] != '\0'
			&& scriptName != noneRef)
		return true;
	return false;
}


Script*
Actor::MergeScripts()
{
	// TODO: order ??
	// Is it correct we merge the scripts ?
	Script* destination = NULL;
	if (IsValid(fActor->script_override))
		_AddScript(destination, fActor->script_override);
	if (IsValid(fActor->script_race))
		_AddScript(destination, fActor->script_race);
	if (IsValid(fActor->script_class))
		_AddScript(destination, fActor->script_class);
	if (IsValid(fActor->script_general))
		_AddScript(destination, fActor->script_general);
	if (IsValid(fActor->script_default))
		_AddScript(destination, fActor->script_default);
	if (IsValid(fActor->script_specific))
		_AddScript(destination, fActor->script_specific);

	//printf("Choose script %s\n", (const char*)fActor->script_specific);
	return destination;
}


bool
Actor::SkipConditions() const
{
	return fDontCheckConditions;
}


void
Actor::StopCheckingConditions()
{
	// TODO: Until reaches destination point
	fDontCheckConditions = true;
}


void
Actor::UpdateSee()
{
	// TODO: Silly implementation: We take a straight line
	// between source and target, and see if there are any unpassable
	// point between them, we also check distance and visibility of
	// the target
	std::list<Object*>::const_iterator start = Core::Get()->Objects().begin();
	std::list<Object*>::const_iterator end = Core::Get()->Objects().end();
	std::list<Object*>::const_iterator i;
	for (i = start; i != end; i++) {
		Actor* target = dynamic_cast<Actor*>(*i);
		// TODO: Take into account any eventual spell
		if (target == NULL || target == this || !target->IsVisible())
			continue;

		//const IE::point thisPosition = Position();
		//const IE::point targetPosition = target->Position();
		// TODO: 200 is an arbitrarily chosen number
		if (Core::Get()->Distance(this, target) < 200 ) {
			// TODO: Check if there are obstacles in the way

			SetSeen(target);
		}
	}
}


void
Actor::SetSeen(Object* object)
{
	CurrentScriptRoundResults()->fSeenList.push_back(object->Name());
	object->SetSeenBy(this);
}


bool
Actor::HasSeen(const Object* object) const
{
	const std::string name = object->Name();
	std::vector<std::string>::const_iterator i;
	i = std::find(LastScriptRoundResults()->fSeenList.begin(),
			LastScriptRoundResults()->fSeenList.end(), name);

	return i != LastScriptRoundResults()->fSeenList.end();
}


void
Actor::SetAnimationAction(int action)
{
	fAction = action;
}


void
Actor::UpdateAnimation(bool ignoreBlocks)
{
	//std::cout << "Actor " << Name() << ": drawing action "<< action;
	//std::cout << ", orientation " << fActor->orientation << std::endl;

	fCurrentAnimation = fAnimationFactory->AnimationFor(
										fAction,
										IE::orientation(fActor->orientation));

	if (fCurrentAnimation != NULL)
		fCurrentAnimation->Next();

	if (fDontCheckConditions == true && fActor->position == fActor->destination)
		fDontCheckConditions = false;
}


void
Actor::MoveToNextPointInPath(bool ignoreBlocks)
{
	if (!fPath->IsEmpty()) {
		IE::point nextPoint = fPath->NextWayPoint();
		_SetOrientation(nextPoint);
		fActor->position = nextPoint;
	}
}


void
Actor::_AddScript(Script*& destination, const res_ref& scriptName)
{
	BCSResource* scriptResource = gResManager->GetBCS(scriptName);
	if (scriptResource == NULL)
		return;
	if (destination == NULL)
		destination = scriptResource->GetScript();
	else
		destination->Add(scriptResource->GetScript());

	gResManager->ReleaseResource(scriptResource);
}


void
Actor::_SetOrientation(const IE::point& nextPoint)
{
	IE::orientation newOrientation = (IE::orientation)fActor->orientation;
	if (nextPoint.x > fActor->position.x) {
		if (nextPoint.y > fActor->position.y)
			newOrientation = IE::ORIENTATION_SE;
		else if (nextPoint.y < fActor->position.y)
			newOrientation = IE::ORIENTATION_NE;
		else
			newOrientation = IE::ORIENTATION_E;
	} else if (nextPoint.x < fActor->position.x) {
		if (nextPoint.y > fActor->position.y)
			newOrientation = IE::ORIENTATION_SW;
		else if (nextPoint.y < fActor->position.y)
			newOrientation = IE::ORIENTATION_NW;
		else
			newOrientation = IE::ORIENTATION_W;
	} else {
		if (nextPoint.y > fActor->position.y)
			newOrientation = IE::ORIENTATION_S;
		else if (nextPoint.y < fActor->position.y)
			newOrientation = IE::ORIENTATION_N;
	}


	fActor->orientation = newOrientation;
}


bool
Actor::IsReachable(const IE::point& pt) const
{
	Room* room = Room::Get();
	int32 state = room->PointSearch(pt);
	switch (state) {
		case 0:
		case 8:
		case 10:
		case 12:
		case 13:
			return false;
		default:
			return true;
	}
}
