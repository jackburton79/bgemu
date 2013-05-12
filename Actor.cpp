#include "Actor.h"
#include "Animation.h"
#include "BamResource.h"
#include "BCSResource.h"
#include "Bitmap.h"
#include "Core.h"
#include "CreResource.h"
#include "Graphics.h"
#include "GraphicsEngine.h"
#include "IDSResource.h"
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
	fCRE(NULL),
	fScript(NULL),
	fOwnsActor(false),
	fDontCheckConditions(false),
	fIsInterruptable(true),
	fFlying(false),
	fPath(NULL)
{
	_Init();
}


Actor::Actor(IE::actor &actor, CREResource* cre)
	:
	Object(actor.name),
	fActor(&actor),
	fCRE(cre),
	fScript(NULL),
	fOwnsActor(false),
	fDontCheckConditions(false),
	fIsInterruptable(true),
	fFlying(false),
	fPath(NULL)
{
	_Init();
}


Actor::Actor(const char* creName, IE::point position, int face)
	:
	Object(creName),
	fActor(NULL),
	fCRE(NULL),
	fScript(NULL),
	fOwnsActor(true),
	fDontCheckConditions(false),
	fFlying(false),
	fPath(NULL)
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
	for (uint32 i = 0; i < kNumAnimations; i++)
		fAnimations[i] = NULL;

	if (fCRE == NULL)
		fCRE = gResManager->GetCRE(fActor->cre);

	std::cout << "colors:" << std::endl << std::dec;
	std::cout << "\tmajor:" << (int)fCRE->Colors().major << std::endl;
	std::cout << "\tminor:" << (int)fCRE->Colors().minor << std::endl;
	std::cout << "\tarmor:" << (int)fCRE->Colors().armor << std::endl;
	std::cout << "\thair:" << (int)fCRE->Colors().hair << std::endl;
	//printf("%d\n", );
	// TODO: Get all scripts ? or just the specific one ?

	if (fCRE == NULL)
		throw "error!!!";

	//printf("%s enum: local %d, global: %d\n", Name(),
		//	fCRE->LocalActorValue(),
			//fCRE->GlobalActorValue());

	fActor->script_override = fCRE->OverrideScriptName();
	fActor->script_class = fCRE->ClassScriptName();
	fActor->script_race = fCRE->RaceScriptName();
	fActor->script_default = fCRE->DefaultScriptName();
	fActor->script_general = fCRE->GeneralScriptName();

	MergeScripts();

	if (fScript != NULL)
		Object::SetScript(fScript);

	fActor->Print();

	//TODO: some orientations are bad!!!
	if (fActor->orientation > IE::ORIENTATION_SE)
		fActor->orientation = 0;

	for (uint32 c = 0; c < kNumAnimations; c++) {
		try {
			fAnimations[c] = new Animation(fCRE, ACT_WALKING, c, fActor->position);
		} catch (...) {
			std::cerr << "Actor::Actor(" << fActor->name << ")";
			std::cerr << ": cannot instantiate Animation ";
			std::cerr << std::hex << fCRE->AnimationID() << std::endl;
			delete fAnimations[c];
			fAnimations[c] = NULL;
		}
	}

	fPath = new PathFinder();
}


Actor::~Actor()
{
	if (fOwnsActor)
		delete fActor;

	gResManager->ReleaseResource(fCRE);

	for (uint32 i = 0; i < kNumAnimations; i++)
		delete fAnimations[i];

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


void
Actor::Draw(GFX::rect area, Bitmap* destBitmap)
{
	Animation* animation = fAnimations[fActor->orientation];
	if (animation == NULL)
		return;

	Frame frame = animation->Frame();
	Bitmap *image = frame.bitmap;
	if (image == NULL)
		return;

	//image->Dump();


	IE::point leftTop = offset_point(Position(), -frame.rect.w / 2,
						-frame.rect.h / 2);
	GFX::rect rect = { leftTop.x, leftTop.y, image->Width(), image->Height() };
	rect = offset_rect(rect, -frame.rect.x, -frame.rect.y);
	if (rects_intersect(area, rect)) {
		rect = offset_rect(rect, -area.x, -area.y);
		// TODO: Mask the actor with the polygons
		//Room::CurrentArea()->GetClipping(rect, )

		GraphicsEngine::BlitBitmap(image, NULL, destBitmap, &rect);
	}
	GraphicsEngine::DeleteBitmap(image);
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
	//if (IsValid(fActor->script_specific))
		//_AddScript(fActor->script_specific);

	//printf("Choose script %s\n", (const char*)fActor->script_specific);
}


::Script *
Actor::Script()
{
	return fScript;
}


void
Actor::SetVariable(const char* name, int32 value)
{
	fVariables[name] = value;
}


int32
Actor::GetVariable(const char* name)
{
	return fVariables[name];
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
	try {
		if (fActor->position != fActor->destination) {
			IE::point nextPoint = fPath->NextWayPoint();
			_SetOrientation(nextPoint);
			if (ignoreBlocks || _IsReachable(nextPoint))
				fActor->position = nextPoint;
		}

		if (fAnimations[fActor->orientation] != NULL)
			fAnimations[fActor->orientation]->Next();
	} catch (...) {
		// No more waypoints
	}

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
	// TODO: Implement correctly
	IE::orientation newOrientation = (IE::orientation)fActor->orientation;
	if (nextPoint.x > fActor->position.x)
		newOrientation = IE::ORIENTATION_E;
	else if (nextPoint.x < fActor->position.x)
		newOrientation = IE::ORIENTATION_W;

	if (nextPoint.y > fActor->position.y)
		newOrientation = (IE::orientation)((int)newOrientation + 1);
	else if (nextPoint.y < fActor->position.y)
		newOrientation = (IE::orientation)((int)newOrientation - 1);

	if (newOrientation < IE::ORIENTATION_S)
		newOrientation = IE::ORIENTATION_S;
	else if (newOrientation > IE::ORIENTATION_SE)
		newOrientation = IE::ORIENTATION_SE;

	fActor->orientation = newOrientation;
}


bool
Actor::_IsReachable(const IE::point& pt)
{
	Room* room = Room::Get();
	const uint32 numPol = room->WED()->CountPolygons();
	for (uint32 i = 0; i < numPol; i++) {
		const Polygon* poly = room->WED()->PolygonAt(i);
		if (!poly->IsHole() && rect_contains(poly->Frame(), pt)) {
			return false;
		}
	}
	return true;
}


/* static */
void
Actor::BlitWithMask(Bitmap* source,
		Bitmap* dest, GFX::rect& rect, IE::polygon& polygonMask)
{

}
