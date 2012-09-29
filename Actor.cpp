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
#include "RectUtils.h"
#include "ResManager.h"
#include "Script.h"

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
	fPath(NULL)
{
	fActor = new IE::actor;
	fActor->cre = creName;
	memcpy(fActor->name, fActor->cre.name, 8);
	fActor->name[8] = 0;
	//fActor->orientation = std::min(face, 7);
	fActor->orientation = 0;
	fActor->position = position;

	_Init();
}


void
Actor::_Init()
{
	for (uint32 i = 0; i < kNumAnimations; i++)
		fAnimations[i] = NULL;

	fCRE = gResManager->GetCRE(fActor->cre);

	//printf("%d\n", fCRE->Colors().major);
	// TODO: Get all scripts ? or just the specific one ?

	if (fCRE == NULL)
		throw "error!!!";

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
			printf("Actor::Actor(): cannot instantiate Animation\n");
			delete fAnimations[c];
			fAnimations[c] = NULL;
		}
	}


	//if (fAnimation != NULL)
		//fAnimation->fBAM->DumpFrames("/home/stefano/dumps");

	fPath = new PathFinder();
}


Actor::~Actor()
{
	if (fOwnsActor)
		delete fActor;
	gResManager->ReleaseResource(fCRE);
	for (uint32 i = 0; i < kNumAnimations; i++)
		delete fAnimations[i];
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
Actor::Draw(GFX::rect area, Bitmap* heightMap)
{
	Animation* animation = fAnimations[fActor->orientation];
	if (animation == NULL)
		return;

	Frame frame = animation->Frame();
	Bitmap *image = frame.bitmap;
	if (image == NULL)
		return;

	IE::point center = offset_point(Position(), -frame.rect.w / 2,
						-frame.rect.h / 2);
	GFX::rect rect = { center.x, center.y, image->Width(), image->Height() };
	rect = offset_rect(rect, -frame.rect.x, -frame.rect.y);

	if (rects_intersect(area, rect)) {
		rect = offset_rect(rect, -area.x, -area.y);
		GraphicsEngine::Get()->BlitToScreen(image, NULL, &rect);
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
	fPath->SetPoints(fActor->position, point);
}


CREResource *
Actor::CRE()
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
Actor::UpdateMove()
{
	try {
		if (fActor->position != fActor->destination) {
			IE::point nextPoint = fPath->NextWayPoint();
			_SetOrientation(nextPoint);
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
	IE::orientation newOrientation;
	if (nextPoint.x > fActor->position.x)
		newOrientation = IE::ORIENTATION_E;
	else
		newOrientation = IE::ORIENTATION_W;

	fActor->orientation = newOrientation;
}
