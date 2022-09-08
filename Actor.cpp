#include "Actor.h"

#include "2DAResource.h"
#include "Action.h"
#include "Animation.h"
#include "AnimationFactory.h"
#include "AreaRoom.h"
#include "BackMap.h"
#include "BamResource.h"
#include "Bitmap.h"
#include "Container.h"
#include "Core.h"
#include "CreResource.h"
#include "Door.h"
#include "Game.h"
#include "GraphicsEngine.h"
#include "IDSResource.h"
#include "ITMResource.h"
#include "Log.h"
#include "Party.h"
#include "PathFind.h"
#include "Polygon.h"
#include "RectUtils.h"
#include "Region.h"
#include "ResManager.h"
#include "Script.h"
#include "SearchMap.h"
#include "TextSupport.h"
#include "TileCell.h"
#include "WedResource.h"

#include <algorithm>
#include <assert.h>
#include <string>


Actor::Actor(IE::actor &actor)
	:
	Object(actor.cre.CString(), Object::ACTOR),
	fActor(&actor),
	fAnimationFactory(NULL),
	fCurrentAnimation(NULL),
	fAnimationAction(ACT_STANDING),
	fNextAnimationAction(ACT_STANDING),
	fAnimationValid(false),
	fAnimationAutoSwitchOnEnd(false),
	fCRE(NULL),
	fOwnsActor(false),
	fColors(NULL),
	fFlying(false),
	fSelected(false),
	fPath(NULL),
	fSpeed(2),
	fTileCell(NULL),
	fRegion(NULL)
{
	_Init();
}


Actor::Actor(IE::actor &actor, CREResource* cre)
	:
	Object(actor.cre.CString(), Object::ACTOR),
	fActor(&actor),
	fAnimationFactory(NULL),
	fCurrentAnimation(NULL),
	fAnimationAction(ACT_STANDING),
	fNextAnimationAction(ACT_STANDING),
	fAnimationValid(false),
	fAnimationAutoSwitchOnEnd(false),
	fCRE(cre),
	fOwnsActor(false),
	fColors(NULL),
	fFlying(false),
	fSelected(false),
	fPath(NULL),
	fSpeed(2),
	fTileCell(NULL),
	fRegion(NULL)
{
	_Init();
}


Actor::Actor(const char* creName, IE::point position, int face)
	:
	Object(creName, Object::ACTOR),
	fActor(new IE::actor),
	fAnimationFactory(NULL),
	fCurrentAnimation(NULL),
	fAnimationAction(ACT_STANDING),
	fNextAnimationAction(ACT_STANDING),
	fAnimationValid(false),
	fAnimationAutoSwitchOnEnd(false),
	fCRE(NULL),
	fOwnsActor(true),
	fColors(NULL),
	fFlying(false),
	fSelected(false),
	fPath(NULL),
	fSpeed(2),
	fTileCell(NULL),
	fRegion(NULL),
	fSelectedRadius(20),
	fSelectedRadiusStep(1)
{
	fActor->cre = creName;
	memcpy(fActor->name, fActor->cre.name, 8);
	fActor->name[8] = 0;
	fActor->orientation = face;
	//fActor->orientation = 0;
	
	fActor->position = position;
	//_SetPositionPrivate(position);

	_Init();
}


std::string
Actor::LongName() const
{
	return fActor->name;
}


void
Actor::_Init()
{
	if (fCRE == NULL) {
		// We need a new instance of the CRE file for every actor,
		// since the state of the actor is written in there
		CREResource* cre = gResManager->GetCRE(fActor->cre);
		if (cre != NULL) {
			fCRE = dynamic_cast<CREResource*>(cre->Clone());
			// TODO: Resource::Clone() only copies the raw data.
			// Anything done in CREResource::Load() will be lost, so it needs to be redone here.
			fCRE->Init();
			gResManager->ReleaseResource(cre);
		}
		if (fCRE == NULL)
			throw std::runtime_error("Actor: CRE file not loaded.");

		_HandleColors();
	}

	// This only makes sense for actors already created once
	if (fCRE->GlobalActorEnum() != uint16(-1))
		SetGlobalID(fCRE->GlobalActorEnum());

#if 0
	Print();
#endif
	// TODO: Get all scripts ? or just the specific one ?

	fAnimationFactory = AnimationFactory::GetFactory(fCRE->AnimationID());

	// TODO: Are we overwriting the actor specific stuff here ?
	fActor->script_override = fCRE->OverrideScriptName();
	fActor->script_class = fCRE->ClassScriptName();
	fActor->script_race = fCRE->RaceScriptName();
	fActor->script_default = fCRE->DefaultScriptName();
	fActor->script_general = fCRE->GeneralScriptName();

	_HandleScripts();

#if 0
	//for (uint32 i = 0; i < kNumItemSlots; i++) {
	uint32 i = 1; // armor slot
	try {
			IE::item item = fCRE->ItemAtSlot(i);
			item.Print();
			ITMResource* itemRes = gResManager->GetITM(item.name);
			if (itemRes != NULL) {
				std::cout << "type: " << std::dec << itemRes->Type() << std::endl;
				std::cout << IDTable::GetDialog(itemRes->DescriptionRef()) << std::endl;
				std::cout << "animation: " << itemRes->Animation() << std::endl;
			}
			gResManager->ReleaseResource(itemRes);
	} catch (...) {
	}
	//}
#endif

	if ((fActor->orientation > IE::ORIENTATION_SE && 
			Core::Get()->Game() == GAME_BALDURSGATE) ||
			fActor->orientation > IE::ORIENTATION_EXT_SSE) {
		std::cerr << "Weird orientation " << fActor->orientation << std::endl;
		fActor->orientation = 0;
	}

	SetActive(true);

	// TODO: Check if it's okay. It's here because it seems it could be uninitialized
	fActor->destination = fActor->position;
	
	if (fCRE->PermanentStatus() == 2048) // STATE_DEAD
		SetAnimationAction(ACT_DEAD);
	else
		SetAnimationAction(ACT_STANDING);
}


Actor::~Actor()
{
	ClearActionList();

	if (fOwnsActor)
		delete fActor;

	gResManager->ReleaseResource(fCRE);

	delete fColors;

	if (fAnimationFactory != NULL) {
		AnimationFactory::ReleaseFactory(fAnimationFactory);
		fAnimationFactory = NULL;
	}

	delete fCurrentAnimation;
	delete fPath;
}


/* virtual */
void
Actor::Print() const
{
	CREResource* cre = CRE();
	if (cre == NULL)
		return;
	std::cout << "Name: " << LongName() << "(" << Name() << ")" << std::endl;
	std::cout << "ENUM: " << cre->GlobalActorEnum() << std::endl;
	std::cout << "Gender: " << IDTable::GenderAt(cre->Gender());
	std::cout << " (" << (int)cre->Gender() << ")" << std::endl;
	std::cout << "Class: " << IDTable::ClassAt(cre->Class());
	std::cout << " (" << (int)cre->Class() << ")" << std::endl;
	std::cout << "Race: " << IDTable::RaceAt(cre->Race());
	std::cout << " (" << (int)cre->Race() << ")" << std::endl;
	std::cout << "EA: " << IDTable::EnemyAllyAt(cre->EnemyAlly());
	std::cout << " (" << (int)cre->EnemyAlly() << ")" << std::endl;
	std::cout << "General: " << IDTable::GeneralAt(cre->General());
	std::cout << " (" << (int)cre->General() << ")" << std::endl;
	std::cout << "Specific: " << IDTable::SpecificAt(cre->Specific());
	std::cout << " (" << (int)cre->Specific() << ")" << std::endl;
	std::cout << "Dialog: " << cre->DialogFile() << std::endl;
	std::cout << "Death Variable: " << cre->DeathVariable() << std::endl;
	std::cout << "Hitpoints:" << cre->CurrentHitPoints() << std::endl;
	std::cout << "Status flags: " << std::dec << cre->PermanentStatus() << std::endl;
	fActor->Print();
	std::cout << "*********" << std::endl;
}


const ::Bitmap*
Actor::Bitmap() const
{
	if (fCurrentAnimation == NULL) {
		std::string message("Actor::Bitmap() (");
		message.append(fCRE->Name()).append(") : No current animation!");
		throw std::runtime_error(message);
	}

	return fCurrentAnimation->Bitmap();
}


// Returns the rect containing the current actor image
IE::rect
Actor::Frame() const
{
	const ::Bitmap* bitmap = Bitmap();
	const GFX::rect& frame = bitmap ? bitmap->Frame() : (GFX::rect){0, 0, 6, 6};
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
	_SetPositionPrivate(position);
	
	// This function is only used to move an actor to a point
	// instantly. So we also need to set its destination to the same
	// point, otherwise it thinks it's walking.
	fActor->destination = position;
}


int
Actor::Orientation() const
{
	return fActor->orientation;
}


void
Actor::SetOrientation(int newOrientation)
{
	uint32 oldOrientation = fActor->orientation;
	fActor->orientation = newOrientation;
	if (newOrientation != (int)oldOrientation)
		fAnimationValid = false;
}


void
Actor::SetOrientation(const IE::point& toPoint)
{
	uint32 oldOrientation = fActor->orientation;
	if (Core::Get()->Game() == GAME_BALDURSGATE)
		_SetOrientation(toPoint);
	else
		_SetOrientationExtended(toPoint);
	if (oldOrientation != fActor->orientation)
		fAnimationValid = false;
}


IE::point
Actor::Destination() const
{
	return fActor->destination;
}


void
Actor::SetDestination(const IE::point& point, bool ignoreSearchMap)
{
	// TODO: If point can't be reached currently it fails without returning
	// the failure to the caller
	if (fPath == NULL) {
		if (ignoreSearchMap)
			fPath = new PathFinder(PathFinder::kStep, Actor::PointPassableTrue);
		else
			fPath = new PathFinder(PathFinder::kStep, AreaRoom::IsPointPassable);
	}
	IE::point destination = fActor->position;
	try {
		destination = fPath->SetPoints(fActor->position, point);
	} catch (...) {
		std::cerr << Log::Red << "Actor::SetDestination() failed!" << Log::Normal << std::endl;
	}
	fActor->destination = destination;
}


void
Actor::ClearDestination()
{
	fActor->destination = fActor->position;
}


void
Actor::Draw(AreaRoom* room) const
{
	_DrawCircle(room);

	IE::point actorPosition = Position();
	actorPosition.y += room->PointHeight(actorPosition) - 8;
	room->DrawBitmap(Bitmap(), actorPosition, true);

	_DrawActorText(room);
	_DrawActorName(room);
}


void
Actor::_DrawActorText(AreaRoom* room) const
{
	std::string text = Text();
	if (!text.empty()) {
		const Font* font = FontRoster::GetFont("TOOLFONT");
		::Bitmap* bitmap = font->GetRenderedString(text, 0);
		IE::point textPoint = Position();
		textPoint.y -= 100;
		room->DrawBitmap(bitmap, textPoint, false);
		bitmap->Release();
	}
}


void
Actor::_DrawActorName(AreaRoom* room) const
{
	std::string text = LongName();
	text.append(" (");
	text.append(Name()).append(")");
	if (!text.empty()) {
		const Font* font = FontRoster::GetFont("TOOLFONT");
		::Bitmap* bitmap = font->GetRenderedString(text, 0);
		IE::point textPoint = Position();
		textPoint.y += 30;
		room->DrawBitmap(bitmap, textPoint, false);
		bitmap->Release();
	}
}


void
Actor::_DrawCircle(AreaRoom* room) const
{
	if (!Core::Get()->CutsceneMode()) {
		::Bitmap* image = room->BackMap()->Image();
		IE::point position = Position();		
		room->ConvertFromArea(position);
		uint32 color = 0;
		if (CRE()->EnemyAlly() < IDTable::EnemyAllyValue("EVILCUTOFF"))
			color = image->MapColor(0, 255, 0);
		else
			color = image->MapColor(255, 0, 0);

		image->Lock();
		image->StrokeCircle(position.x, position.y,
							fSelected ? fSelectedRadius : 10, color);
		image->Unlock();
	}	
}


/* virtual */
IE::point
Actor::NearestPoint(const IE::point& start) const
{
	const IE::point restriction = RestrictionDistance();
	IE::point targetPoint = Position();
	if (start.x < targetPoint.x)
		targetPoint.x -= restriction.x;
	else if (start.x > targetPoint.x)
		targetPoint.x += restriction.x;
	if (start.y < targetPoint.y)
		targetPoint.y -= restriction.y;
	else if (start.y > targetPoint.y)
		targetPoint.y += restriction.y;
	return targetPoint;
}


bool
Actor::IsWalking() const
{
	return fActor->destination != fActor->position;
}


void
Actor::SetRegion(Region* region)
{
	fRegion = region;
}


Region*
Actor::CurrentRegion() const
{
	return fRegion;
}


bool
Actor::IsEqual(const Actor* actorB) const
{
	if (actorB == NULL)
		return false;

	CREResource* creA = this->CRE();
	CREResource* creB = actorB->CRE();

	if (::strcasecmp(this->Name(), actorB->Name()) == 0
		&& (creA->Class() == creB->Class())
		&& (creA->Race() == creB->Race())
		&& (creA->Alignment() == creB->Alignment())
		&& (creA->Gender() == creB->Gender())
		&& (creA->General() == creB->General())
		&& (creA->Specific() == creB->Specific())
		&& (creA->EnemyAlly(), creB->EnemyAlly()))
		return true;
	return false;
}


bool
Actor::IsEnemyOf(const Actor* actor) const
{
	// TODO: Implement correctly
	uint8 enemy = IDTable::EnemyAllyValue("ENEM");
	uint8 pc = IDTable::EnemyAllyValue("PC");
	// TODO: Is this correct ? I have no idea.
	return (actor->IsEnemyAlly(enemy) 	&& IsEnemyAlly(pc))
			|| (actor->IsEnemyAlly(pc) && IsEnemyAlly(enemy));
}


bool
Actor::IsName(const char* name) const
{
	if (name[0] == '\0' || !strcasecmp(name, Name()))
		return true;
	return false;
}


bool
Actor::IsClass(int c) const
{
	CREResource* cre = CRE();
	if (c == 0 || c == cre->Class())
		return true;

	return false;
}


bool
Actor::IsRace(int race) const
{
	CREResource* cre = CRE();
	if (race == 0 || race == cre->Race())
		return true;

	return false;
}


bool
Actor::IsGender(int gender) const
{
	CREResource* cre = CRE();
	if (gender == 0 || gender == cre->Gender())
		return true;

	return false;
}


bool
Actor::IsGeneral(int general) const
{
	CREResource* cre = CRE();
	if (general == 0 || general == cre->General())
		return true;

	return false;
}


bool
Actor::IsSpecific(int specific) const
{
	CREResource* cre = CRE();
	if (specific == 0 || specific == cre->Specific())
		return true;

	return false;
}


bool
Actor::IsAlignment(int alignment) const
{
	CREResource* cre = CRE();
	if (alignment == 0 || alignment == cre->Alignment())
		return true;

	return false;
}


bool
Actor::IsEnemyAlly(int ea) const
{
	if (ea == 0)
		return true;

	CREResource* cre = this->CRE();
	if (ea == cre->EnemyAlly())
		return true;

	std::string eaString = IDTable::EnemyAllyAt(ea);

	if (eaString == "PC") {
		if (Game::Get()->Party()->HasActor(this))
			return true;
	} else if (eaString == "GOODCUTOFF") {
		if (cre->EnemyAlly() <= ea)
			return true;
	} else if (eaString == "EVILCUTOFF") {
		if (cre->EnemyAlly() >= ea)
			return true;
	}

	return false;
}


void
Actor::SetEnemyAlly(int ea)
{
	CRE()->SetEnemyAlly(ea);
}


bool
Actor::IsState(int state) const
{
	if (CRE()->PermanentStatus() & state)
		return true;

	return false;
}


// Checks if this object matches with the specified object_node.
// Also keeps wildcards in consideration. Used for triggers.
bool
Actor::MatchNode(object_params* node) const
{
	if (IsName(node->name)
		&& IsClass(node->classs)
		&& IsRace(node->race)
		&& IsAlignment(node->alignment)
		&& IsGender(node->gender)
		&& IsGeneral(node->general)
		&& IsSpecific(node->specific)
		&& IsEnemyAlly(node->ea))
		return true;

	return false;
}


bool
Actor::Spawned() const
{
	return fActor->spawned != 0;
}


std::string
Actor::ArmorAnimation() const
{
	// TODO: Refactor: items should be loaded elsewhere
	IE::item armor;
	if (fCRE->GetItemAtSlot(1, armor)) {
		ITMResource* itm = gResManager->GetITM(armor.name);
		if (itm != NULL) {
			std::string animationString = itm->Animation();
			gResManager->ReleaseResource(itm);
			return animationString;
		}
	}

	return "1";
}


std::string
Actor::WeaponAnimation() const
{
	// TODO: Refactor: items should be loaded elsewhere
	IE::item weapon;
	if (fCRE->GetItemAtSlot(35, weapon)) {
		ITMResource* itm = gResManager->GetITM(weapon.name);
		if (itm != NULL) {
			std::string animationString = itm->Animation();
			gResManager->ReleaseResource(itm);
			return animationString;
		}
	}

	return "";
}


bool
Actor::InParty() const
{
	return Game::Get()->Party()->HasActor(this);
}


void
Actor::IncrementNumTimesTalkedTo()
{
	fActor->num_times_talked_to++;
}


uint32
Actor::NumTimesTalkedTo() const
{
	return fActor->num_times_talked_to;
}


/* virtual */
IE::point
Actor::RestrictionDistance() const
{
	IE::point point = {
		(int16)fActor->movement_restriction_distance,
		(int16)fActor->movement_restriction_distance
	};
	return point;
}


/* virtual */
void
Actor::ClickedOn(Object* target)
{
	if (target == NULL)
		return;

	target->Clicked(this);
	
	// TODO: Add a "mode" to the ClickedOn method, to distinguish
	// an attack from a dialog start, etc

	if (Door* door = dynamic_cast<Door*>(target)) {
		action_params* actionParams = new action_params(Name(), door->Name());
		AddAction(new ActionWalkToObject(this, actionParams));
		AddAction(new ActionOpenDoor(this, actionParams));
		actionParams->Release();
	} else if (Actor* actor = dynamic_cast<Actor*>(target)) {
		action_params* actionParams = new action_params(actor->Name(), Name());
		AddAction(new ActionDialog(this, actionParams));
		actionParams->Release();
	} /* else if (Container* container = dynamic_cast<Container*>(target)) {
		Action* walkTo = new WalkToObject(this, container);
		AddAction(walkTo);
	}*/
}


void
Actor::Shout(int number)
{
	// TODO: Not sure if handling shouts as triggers is correct
	// Moreover: we need to track the shout number
	trigger_entry shout("shout");
	AddTrigger(shout);
	// Track who has heard this shout
	for (int32 a = 0; a < Area()->ActorsCount(); a++) {
		Actor* actor = Area()->ActorAt(a);
		if (Area()->Distance(actor, this) < 200)
			actor->AddTrigger(trigger_entry("LastHeardBy", this));
	}
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
Actor::Select(bool select)
{
	fSelected = select;
}


bool
Actor::IsSelected() const
{
	return fSelected;
}


CREResource*
Actor::CRE() const
{
	return fCRE;
}


void
Actor::_HandleScripts()
{
	AddScript(Core::ExtractScript(fActor->script_override));
	// What is the area script ?
	//AddScript(Core::ExtractScript(fActor->script_area));
	AddScript(Core::ExtractScript(fActor->script_specific));		
	AddScript(Core::ExtractScript(fActor->script_class));
	AddScript(Core::ExtractScript(fActor->script_race));
	AddScript(Core::ExtractScript(fActor->script_general));
	AddScript(Core::ExtractScript(fActor->script_default));
}


void
Actor::AttackTarget(Actor* target)
{
	trigger_entry triggerEntry("AttackedBy", this);
	target->AddTrigger(triggerEntry);
}


bool
Actor::CanSee(Object* target)
{
	// TODO: Take into account any eventual spell
	if (target == NULL || target == this || !target->IsVisible())
		return false;
	//const IE::point thisPosition = Position();
	//const IE::point targetPosition = target->Position();
	// TODO: 200 is an arbitrarily chosen number
	if (Area()->Distance(this, target) < 200 ) {
		// TODO: Check if there are obstacles in the way
		trigger_entry entry("LastSeen", target);
		AddTrigger(entry);
		return true;
	}
	return false;
}


/* virtual */
void
Actor::Update(bool scripts)
{
	if (IsActionListEmpty() && IsWalking()) {
		SetAnimationAction(ACT_WALKING);
		MoveToNextPointInPath(false);
		SetWaitTime(1);
	}

	Object::Update(scripts);
	UpdateTileCell();
	UpdateAnimation(IsFlying());
	if (fSelected) {
		if (fSelectedRadius > 22) {
			fSelectedRadiusStep = -1;
		} else if (fSelectedRadius < 18) {
			fSelectedRadiusStep = 1;
		}
		fSelectedRadius += fSelectedRadiusStep;
	}
}


int
Actor::AnimationAction() const
{
	return fAnimationAction;
}


void
Actor::SetAnimationAction(int action)
{
	if (fAnimationAction != action) {
		fAnimationAction = action;
		fAnimationValid = false;
		switch (action) {
			case ACT_CAST_SPELL_RELEASE:
				fNextAnimationAction = ACT_STANDING;
				fAnimationAutoSwitchOnEnd = true;
				break;
			case ACT_DIE:
				fNextAnimationAction = ACT_DEAD;
				fAnimationAutoSwitchOnEnd = true;
				break;
			default:
				fNextAnimationAction = ACT_STANDING;
				fAnimationAutoSwitchOnEnd = false;
				break;
		}
	}
}


void
Actor::UpdateAnimation(bool ignoreBlocks)
{
	if (!fAnimationValid) {
		delete fCurrentAnimation;
		fCurrentAnimation = NULL;
		if (fAnimationFactory != NULL) {
			fCurrentAnimation = fAnimationFactory->AnimationFor(this, fColors);
		}
		fAnimationValid = true;
	} else if (fCurrentAnimation != NULL) {
		if (fCurrentAnimation->IsLastFrame()) {
			if (fAnimationAutoSwitchOnEnd) {
				fAnimationAutoSwitchOnEnd = false;
				fAnimationAction = fNextAnimationAction;
				fCurrentAnimation = fAnimationFactory->AnimationFor(this, fColors);
			}
			if (fAnimationAction != ACT_DEAD) {
				fCurrentAnimation->NextFrame();
			}
		} else
			fCurrentAnimation->NextFrame();
		/*if ((fAnimationAction != ACT_DIE && fAnimationAction != ACT_CAST_SPELL_RELEASE)
				|| !fCurrentAnimation->IsLastFrame())
			fCurrentAnimation->NextFrame();*/
	}
}


void
Actor::MoveToNextPointInPath(bool ignoreBlocks)
{
	if (fPath == NULL)
		return;

	if (!fPath->IsEmpty()) {
		IE::point nextPoint = fPath->NextWayPoint();
		SetOrientation(nextPoint);
		_SetPositionPrivate(nextPoint);
		SetAnimationAction(ACT_WALKING);
		return;
	}

	if (Position() == Destination())
		SetAnimationAction(ACT_STANDING);

	if (fPath->IsEmpty()) {
		SetAnimationAction(ACT_STANDING);
		delete fPath;
		fPath = NULL;
	}
}


void
Actor::_SetOrientation(const IE::point& nextPoint)
{
	int newOrientation = fActor->orientation;
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


void
Actor::_SetOrientationExtended(const IE::point& nextPoint)
{
	int newOrientation = fActor->orientation;
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

	fActor->orientation = newOrientation;
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


void
Actor::UpdateTileCell()
{
	BackMap* backMap = Area()->BackMap();
	if (backMap == NULL)
		return;

	::TileCell* oldTileCell = fTileCell;
	::TileCell* newTileCell = backMap->TileAtPoint(Position());

	if (oldTileCell != newTileCell) {
		if (oldTileCell != NULL)
			oldTileCell->ActorExitedCell(this);
		fTileCell = newTileCell;
		if (newTileCell != NULL)
			newTileCell->ActorEnteredCell(this);
	}
}


// Called by TileCell on destruction
void
Actor::SetTileCell(::TileCell* cell)
{
	fTileCell = cell;
}


void
Actor::SetText(const std::string& string)
{
	fText = string;
}


std::string
Actor::Text() const
{
	return fText;
}


void
Actor::_SetPositionPrivate(const IE::point& point)
{
	AreaRoom* room = Area();
	if (room != NULL) {
		room->SearchMap()->ClearPoint(fActor->position.x, fActor->position.y);
		TileCell* tile = room->BackMap()->TileAtPoint(fActor->position);
		if (tile != NULL)
			tile->ActorExitedCell(this);
	}

	fActor->position = point;

	if (room != NULL) {
		room->SearchMap()->SetPoint(fActor->position.x, fActor->position.y);
		room->BackMap()->TileAtPoint(point)->ActorEnteredCell(this);
	}
}


void
Actor::_HandleColors()
{
	assert(fColors == NULL);
	TWODAResource* randColors = gResManager->Get2DA("RANDCOLR");
	if (randColors != NULL) {
		CREColors originalColors = CRE()->Colors();
		fColors = new CREColors;
		fColors->hair = _GetRandomColor(randColors, originalColors.hair);
		fColors->leather = _GetRandomColor(randColors, originalColors.leather);
		fColors->armor = _GetRandomColor(randColors, originalColors.armor);
		fColors->metal = _GetRandomColor(randColors, originalColors.metal);
		fColors->major = _GetRandomColor(randColors, originalColors.major);
		fColors->minor = _GetRandomColor(randColors, originalColors.minor);
		fColors->skin = _GetRandomColor(randColors, originalColors.skin);
		gResManager->ReleaseResource(randColors);
	}
}


uint8
Actor::_GetRandomColor(TWODAResource* randColors, uint8 index) const
{	
	uint8 num = index;
	// get column requested index
	for (int i = 0; i < randColors->CountColumns(); i++) {
		uint16 value = randColors->IntegerValueAt(0, i);
		if (value == index) {
			int rndNumber = Core::RandomNumber(1, randColors->CountRows() - 1);
			num = randColors->IntegerValueAt(rndNumber, i);
			break;
		}
	}

	return num;
}


bool
Actor::EvaluateDialogTriggers(std::vector<trigger_params*>& triggers)
{
	bool debug = 0;
#if 1
	debug = 1;
#endif
	if (triggers.size() == 0)
		return false;

	if (debug)
		std::cout << "IF ";

	for (std::vector<trigger_params*>::iterator i = triggers.begin();
			i != triggers.end(); i++) {
		int orTrig = 0;
		trigger_params* triggerNode = *i;
		if (debug)
			triggerNode->Print();
		if (!Script::EvaluateTrigger(this, triggerNode, orTrig))
			return false;
		if (debug) {
			if (orTrig)
				std::cout << " OR " << std::endl;
			else
				std::cout << " AND " << std::endl;
		}
	}
	return true;
}
