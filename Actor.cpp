#include "Actor.h"

#include "Action.h"
#include "Animation.h"
#include "AnimationFactory.h"
#include "AreaRoom.h"
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
#include "RoundResults.h"
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
	fAnimationValid = false;

	if (fCRE == NULL) {
		// We need a new instance of the CRE file for every actor,
		// since the state of the actor is written in there
		CREResource* cre = gResManager->GetCRE(fActor->cre);
		if (cre != NULL) {
			fCRE = dynamic_cast<CREResource*>(cre->Clone());
			gResManager->ReleaseResource(cre);
		}
		if (fCRE == NULL)
			throw "Actor: CRE file not loaded.";
	}

	// TODO: Get all scripts ? or just the specific one ?

	fAnimationFactory = AnimationFactory::GetFactory(fCRE->AnimationID());

	CREColors colors = fCRE->Colors();
	std::cout << "colors:" << std::endl << std::dec;
	std::cout << "\tmajor:" << (int)colors.major << std::endl;
	std::cout << "\tminor:" << (int)colors.minor << std::endl;
	std::cout << "\tarmor:" << (int)colors.armor << std::endl;
	std::cout << "\thair:" << (int)colors.hair << std::endl;
	//std::cout << std::dec;
	//std::cout << Name() << " enum: local: " << fCRE->LocalActorEnum();
	//std::cout << ", global: " << fCRE->GlobalActorEnum() << std::endl;

	// TODO: Are we overwriting the actor specific stuff here ?
	fActor->script_override = fCRE->OverrideScriptName();
	fActor->script_class = fCRE->ClassScriptName();
	fActor->script_race = fCRE->RaceScriptName();
	fActor->script_default = fCRE->DefaultScriptName();
	fActor->script_general = fCRE->GeneralScriptName();

	Script* script = MergeScripts();
	SetScript(script);

	//fActor->Print();
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

	// TODO: Check if it's okay. It's here because it seems it could be uninitialized
	fActor->destination = fActor->position;

	fPath = new PathFinder(PathFinder::kStep, RoomBase::IsPointPassable);
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

	delete fCurrentAnimation;
	delete fPath;
}


// Returns true if an actor was just instantiated
// false if it was already existing (loaded from save)
bool
Actor::IsNew() const
{
	return fCRE != NULL && fCRE->GlobalActorEnum() != 0;
}


const ::Bitmap*
Actor::Bitmap() const
{
	if (fCurrentAnimation == NULL) {
		/*std::string message("Actor::Bitmap() (");
		message.append(fCRE->Name()).append(") : No current animation!");
		throw message;*/
		return NULL;
	}

	return fCurrentAnimation->Bitmap();
}


// Returns the rect containing the current actor image
IE::rect
Actor::Frame() const
{
	const GFX::rect& frame = Bitmap()->Frame();
	IE::point leftTop = offset_point(Position(),
								-(frame.x + frame.w / 2),
								-(frame.y + frame.h / 2));

	IE::rect rect = {
			leftTop.x,
			leftTop.y,
			(int16)(leftTop.x + frame.w),
			(int16)(leftTop.y + frame.h)
	};

	return rect;
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


int
Actor::Orientation() const
{
	return fActor->orientation;
}


void
Actor::SetOrientation(int o)
{
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


bool
Actor::Spawned() const
{
	return fActor->spawned != 0;
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
	//CurrentScriptRoundResults()->fShouted = number;
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
	std::list<Reference<Object> >::const_iterator i;
	std::list<Reference<Object> > actorsList;
	Core::Get()->GetObjectList(actorsList);
	
	std::list<Reference<Object> >::const_iterator start = actorsList.begin();
	std::list<Reference<Object> >::const_iterator end = actorsList.end();
	
	for (i = start; i != end; i++) {
		Actor* target = dynamic_cast<Actor*>(i->Target());
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
	Core::Get()->RoundResults()->ActorSaw(this, dynamic_cast<const Actor*>(object));
}


bool
Actor::HasSeen(const Object* object) const
{
	return Core::Get()->LastRoundResults()->WasActorSeenBy(this, dynamic_cast<const Actor*>(object));
}


void
Actor::SetAnimationAction(int action)
{
	if (fAction != action) {
		fAction = action;
		fAnimationValid = false;
	}
}


void
Actor::UpdateAnimation(bool ignoreBlocks)
{
	if (!fAnimationValid) {
		delete fCurrentAnimation;
		fCurrentAnimation = fAnimationFactory->AnimationFor(
										fAction,
										fActor->orientation);
		fAnimationValid = true;
	} else if (fCurrentAnimation != NULL)
		fCurrentAnimation->Next();

	if (fDontCheckConditions == true && fActor->position == fActor->destination)
		fDontCheckConditions = false;
}


void
Actor::MoveToNextPointInPath(bool ignoreBlocks)
{
	if (!fPath->IsEmpty()) {
		IE::point nextPoint = fPath->NextWayPoint();
		if (Core::Get()->Game() == GAME_BALDURSGATE)
			_SetOrientation(nextPoint);
		else
			_SetOrientationExtended(nextPoint);
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
	int oldOrientation = fActor->orientation;
	int newOrientation = oldOrientation;
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
	if (newOrientation != oldOrientation)
		fAnimationValid = false;
}


void
Actor::_SetOrientationExtended(const IE::point& nextPoint)
{
	int oldOrientation = fActor->orientation;
	int newOrientation = oldOrientation;
	if (nextPoint.x > fActor->position.x) {
		if (nextPoint.y > fActor->position.y)
			newOrientation = IE::ORIENTATION_EXT_SE;
		else if (nextPoint.y < fActor->position.y)
			newOrientation = IE::ORIENTATION_EXT_NE;
		else
			newOrientation = IE::ORIENTATION_EXT_E;
	} else if (nextPoint.x < fActor->position.x) {
		if (nextPoint.y > fActor->position.y)
			newOrientation = IE::ORIENTATION_EXT_SW;
		else if (nextPoint.y < fActor->position.y)
			newOrientation = IE::ORIENTATION_EXT_NW;
		else
			newOrientation = IE::ORIENTATION_EXT_W;
	} else {
		if (nextPoint.y > fActor->position.y)
			newOrientation = IE::ORIENTATION_EXT_S;
		else if (nextPoint.y < fActor->position.y)
			newOrientation = IE::ORIENTATION_EXT_N;
	}

	//std::cout << Name() << ": Orientation: " << std::dec << newOrientation << std::endl;
	fActor->orientation = newOrientation;
	if (newOrientation != oldOrientation)
		fAnimationValid = false;
}


bool
Actor::IsReachable(const IE::point& pt) const
{
	/*RoomContainer* room = Core::Get()->CurrentRoom();
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
	}*/
	return false;
}
