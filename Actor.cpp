#include "Actor.h"
#include "Animation.h"
#include "AnimationFactory.h"
#include "BamResource.h"
#include "BCSResource.h"
#include "Bitmap.h"
#include "Core.h"
#include "CreResource.h"
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

#include <assert.h>
#include <string>

std::vector<Actor*> Actor::sActors;

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
	fEnemyOfEveryone = false;
	fCurrentAnimation = NULL;

	if (fCRE == NULL)
		fCRE = gResManager->GetCRE(fActor->cre);

	//printf("%d\n", );
	// TODO: Get all scripts ? or just the specific one ?

	if (fCRE == NULL)
		throw "error!!!";

	std::string baseName = IDTable::AniSndAt(fCRE->AnimationID());
	fAnimationFactory = AnimationFactory::GetFactory(baseName.c_str());

	std::cout << "colors:" << std::endl << std::dec;
	std::cout << "\tmajor:" << (int)fCRE->Colors().major << std::endl;
	std::cout << "\tminor:" << (int)fCRE->Colors().minor << std::endl;
	std::cout << "\tarmor:" << (int)fCRE->Colors().armor << std::endl;
	std::cout << "\thair:" << (int)fCRE->Colors().hair << std::endl;
	//printf("%s enum: local %d, global: %d\n", Name(),
		//	fCRE->LocalActorValue(),
			//fCRE->GlobalActorValue());

	// TODO: Are we overwriting the actor specific stuff here ?
	fActor->script_override = fCRE->OverrideScriptName();
	fActor->script_class = fCRE->ClassScriptName();
	fActor->script_race = fCRE->RaceScriptName();
	fActor->script_default = fCRE->DefaultScriptName();
	fActor->script_general = fCRE->GeneralScriptName();

	MergeScripts();

	if (fScript != NULL)
		SetScript(fScript);

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

	fPath = new PathFinder();
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


/* static */
void
Actor::Add(Actor* actor)
{
	sActors.push_back(actor);
}


/* static */
void
Actor::Remove(const char* name)
{

}


/* static */
Actor*
Actor::GetByIndex(uint32 i)
{
	return sActors[i];
}


/* static */
Actor*
Actor::GetByName(const char* name)
{
	std::vector<Actor*>::const_iterator i;
	for (i = sActors.begin(); i != sActors.end(); i++) {
		if (!strcmp((*i)->Name(), name))
				return *i;
	}
	return NULL;
}


/* static */
std::vector<Actor*>&
Actor::List()
{
	return sActors;
}


const char *
Actor::Name() const
{
	return (const char *)fActor->name;
}


const ::Bitmap*
Actor::Bitmap() const
{
	if (fCurrentAnimation == NULL)
		throw "Actor::Frame(): No current animation!";

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

	return GFX::rect(leftTop.x, leftTop.y, frame.w, frame.h);;
}


IE::point
Actor::Position() const
{
	return fActor->position;
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
	fActor->destination = point;
	fPath->SetPoints(fActor->position, fActor->destination);
}


void
Actor::Shout(int number)
{
	CurrentScriptRoundResults()->fShouted = number;
}


void
Actor::SetIsEnemyOfEveryone(bool enemy)
{
	fEnemyOfEveryone = true;
}


bool
Actor::IsEnemyOfEveryone() const
{
	return fEnemyOfEveryone;
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


void
Actor::MergeScripts()
{
	// TODO: order ??
	// Is it correct we merge the scripts ?
	if (IsValid(fActor->script_override))
		_AddScript(fActor->script_override);
	if (IsValid(fActor->script_race))
		_AddScript(fActor->script_race);
	if (IsValid(fActor->script_class))
		_AddScript(fActor->script_class);
	if (IsValid(fActor->script_general))
		_AddScript(fActor->script_general);
	if (IsValid(fActor->script_default))
		_AddScript(fActor->script_default);
	if (IsValid(fActor->script_specific))
		_AddScript(fActor->script_specific);

	//printf("Choose script %s\n", (const char*)fActor->script_specific);
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
Actor::UpdateMove(bool ignoreBlocks)
{
	if (fActor->position != fActor->destination) {
		IE::point nextPoint;
		for (int32 i = 0; i < fSpeed; i++)
			nextPoint = fPath->NextWayPoint();

		_SetOrientation(fActor->destination);
		// TODO: We should do this, since the path to the destination
		// could involve facing to a different direction than
		// the real destination point
		//_SetOrientation(nextPoint);

		if (ignoreBlocks || _IsReachable(nextPoint))
			fActor->position = nextPoint;
	}

	int action = ACT_STANDING;
	if (fActor->position != fActor->destination)
		action = ACT_WALKING;

	//std::cout << "Actor " << Name() << ": drawing action "<< action;
	//std::cout << ", orientation " << fActor->orientation << std::endl;

	fCurrentAnimation = fAnimationFactory->AnimationFor(
										action,
										IE::orientation(fActor->orientation));

	if (fCurrentAnimation != NULL)
		fCurrentAnimation->Next();

	if (fDontCheckConditions == true && fActor->position == fActor->destination)
		fDontCheckConditions = false;
}


void
Actor::_AddScript(const res_ref& scriptName)
{
	//printf("Actor::_AddScript(%s)\n", (const char*)scriptName);
	BCSResource* scriptResource = gResManager->GetBCS(scriptName);
	if (scriptResource == NULL)
		return;
	if (fScript == NULL)
		fScript = scriptResource->GetScript();
	else
		fScript->Add(scriptResource->GetScript());

	gResManager->ReleaseResource(scriptResource);
}


void
Actor::_SetOrientation(const IE::point& nextPoint)
{
	IE::orientation newOrientation = (IE::orientation)fActor->orientation;
	if (nextPoint.x > fActor->position.x + 5) {
		if (nextPoint.y > fActor->position.y + 5)
			newOrientation = IE::ORIENTATION_SE;
		else if (nextPoint.y < fActor->position.y - 5)
			newOrientation = IE::ORIENTATION_NE;
		else
			newOrientation = IE::ORIENTATION_E;
	} else if (nextPoint.x < fActor->position.x - 5) {
		if (nextPoint.y > fActor->position.y + 5)
			newOrientation = IE::ORIENTATION_SW;
		else if (nextPoint.y < fActor->position.y - 5)
			newOrientation = IE::ORIENTATION_NW;
		else
			newOrientation = IE::ORIENTATION_W;
	} else {
		if (nextPoint.y > fActor->position.y + 5)
			newOrientation = IE::ORIENTATION_S;
		else if (nextPoint.y < fActor->position.y - 5)
			newOrientation = IE::ORIENTATION_N;
	}


	fActor->orientation = newOrientation;
}


bool
Actor::_IsReachable(const IE::point& pt)
{
	Room* room = Room::Get();
	const uint32 numPol = room->WED()->CountPolygons();
	for (uint32 i = 0; i < numPol; i++) {
		const Polygon* poly = room->WED()->PolygonAt(i);
		// TODO:
		if (rect_contains(poly->Frame(), pt)) {
			return false;
		}
	}
	return true;
}


/* static */
void
Actor::BlitWithMask(::Bitmap* source,
		::Bitmap* dest, GFX::rect& rect, IE::polygon& polygonMask)
{

}
