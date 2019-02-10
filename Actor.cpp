#include "Actor.h"

#include "2DAResource.h"
#include "Action.h"
#include "Animation.h"
#include "AnimationFactory.h"
#include "AreaRoom.h"
#include "BackMap.h"
#include "BamResource.h"
#include "BCSResource.h"
#include "Bitmap.h"
#include "Container.h"
#include "Core.h"
#include "CreResource.h"
#include "DLGResource.h"
#include "Door.h"
#include "Game.h"
#include "GraphicsEngine.h"
#include "IDSResource.h"
#include "ITMResource.h"
#include "Party.h"
#include "PathFind.h"
#include "Polygon.h"
#include "RectUtils.h"
#include "Region.h"
#include "ResManager.h"
#include "RoundResults.h"
#include "Script.h"
#include "SearchMap.h"
#include "TileCell.h"
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
	
	_SetPositionPrivate(position);

	_Init();
}


void
Actor::_Init()
{
	fSelected = false;
	fCurrentAnimation = NULL;
	fAnimationValid = false;
	fTileCell = NULL;
	fColors = NULL;
	fRegion = NULL;

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
			throw "Actor: CRE file not loaded.";

		_HandleColors();
	}

	// TODO: Get all scripts ? or just the specific one ?

	fAnimationFactory = AnimationFactory::GetFactory(fCRE->AnimationID());
	if (fAnimationFactory == NULL) {
		std::cerr << "No animation factory " << CRE()->Name();
		std::cerr << " (" << CRE()->AnimationID() << ")" << std::endl;
	}
	//std::cout << std::dec;
	//std::cout << Name() << " enum: local: " << fCRE->LocalActorEnum();
	//std::cout << ", global: " << fCRE->GlobalActorEnum() << std::endl;

	// TODO: Are we overwriting the actor specific stuff here ?
	fActor->script_override = fCRE->OverrideScriptName();
	fActor->script_class = fCRE->ClassScriptName();
	fActor->script_race = fCRE->RaceScriptName();
	fActor->script_default = fCRE->DefaultScriptName();
	fActor->script_general = fCRE->GeneralScriptName();

	_HandleScripts();

#if 0
	//fActor->Print();
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

	//TODO: some orientations are bad. Why?!?!?!
	if ((fActor->orientation > IE::ORIENTATION_SE && 
			Core::Get()->Game() == GAME_BALDURSGATE) ||
			fActor->orientation > IE::ORIENTATION_EXT_SSE) {
		std::cerr << "Weird orientation " << fActor->orientation << std::endl;
		fActor->orientation = 0;
	}

	// TODO: Check if it's okay. It's here because it seems it could be uninitialized
	fActor->destination = fActor->position;

	fPath = new PathFinder(PathFinder::kStep, AreaRoom::IsPointPassable);
	
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


void
Actor::Print() const
{
	CREResource* cre = CRE();
	if (cre == NULL)
		return;
	std::cout << "*** " << Name() << " ***" << std::endl;
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
	std::cout << "*********" << std::endl;
}


/* virtual */
uint16
Actor::GlobalID() const
{
	return fCRE != NULL ? fCRE->GlobalActorEnum() : 0;
}


// Returns true if an actor was just instantiated
// false if it was already existing (loaded from save)
bool
Actor::IsNew() const
{
	return fCRE != NULL && fCRE->GlobalActorEnum() == (uint16)-1;
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
	const ::Bitmap* bitmap = Bitmap();
	const GFX::rect& frame = bitmap ? bitmap->Frame() : (GFX::rect){0, 0, 6, 0};
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


void
Actor::ClearDestination()
{
	fActor->destination = fActor->position;
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


bool
Actor::HandleWalking()
{
	if (IsWalking()) {
		SetAnimationAction(ACT_WALKING);
		MoveToNextPointInPath(false);
		if (!IsWalking())
			SetAnimationAction(ACT_STANDING);
		return true;
	}
	return false;
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


/* static */
bool
Actor::IsDummy(const object_node* node)
{
	if (node->ea == 0
			&& node->general == 0
			&& node->race == 0
			&& node->classs == 0
			&& node->specific == 0
			&& node->gender == 0
			&& node->alignment == 0
			//&& node->faction == 0
			//&& node->team == 0
			&& node->identifiers[0] == 0
			&& node->identifiers[1] == 0
			&& node->identifiers[2] == 0
			&& node->identifiers[3] == 0
			&& node->identifiers[4] == 0
			&& node->name[0] == '\0'
			) {
		return true;
	}

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
Actor::MatchNode(object_node* node) const
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
		if (Core::Get()->Distance(this, door) > 10)
			SetDestination(door->NearestPoint(Position()));
		else
			door->Toggle();
	}/* else if (Actor* actor = dynamic_cast<Actor*>(target)) {
		Attack* attackAction = new Attack(this, actor);
		AddAction(attackAction);
	} else if (Container* container = dynamic_cast<Container*>(target)) {
		Action* walkTo = new WalkToObject(this, container);
		AddAction(walkTo);
	}*/
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

/*
static bool
IsValid(const res_ref& scriptName)
{
	res_ref noneRef = "None";
	if (scriptName.name[0] != '\0'
			&& scriptName != noneRef)
		return true;
	return false;
}
*/

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
	Core::Get()->RoundResults()->SetActorAttacked(this, target);
}


bool
Actor::WasAttackedBy(object_node* node)
{
	return Core::Get()->LastRoundResults()->WasActorAttackedBy(this, node);
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
	ActorsList::const_iterator i;
	ActorsList actorsList;
	Core::Get()->GetActorsList(actorsList);
	
	ActorsList::const_iterator start = actorsList.begin();
	ActorsList::const_iterator end = actorsList.end();
	
	for (i = start; i != end; i++) {
		Actor* target = *i;
		// TODO: Take into account any eventual spell
		if (target == NULL || target == this || !target->IsVisible())
			continue;

		//const IE::point thisPosition = Position();
		//const IE::point targetPosition = target->Position();
		// TODO: 200 is an arbitrarily chosen number
		if (Core::Get()->Distance(this, target) < 200 ) {
			// TODO: Check if there are obstacles in the way
			SetSeen(target);
			//std::cout << this->Name() << " SAW " << target->Name() << std::endl;
		}
	}
}


void
Actor::SetSeen(Object* object)
{
	Core::Get()->RoundResults()->SetActorSaw(this, dynamic_cast<const Actor*>(object));
}


bool
Actor::HasSeen(const Object* object) const
{
	return Core::Get()->LastRoundResults()->WasActorSeenBy(dynamic_cast<const Actor*>(object), this);
}


/* virtual */
void
Actor::Update(bool scripts)
{
	_CheckRegion();
	
	Object::Update(scripts);
	UpdateTileCell();

	UpdateAnimation(IsFlying());
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
		if (fAnimationFactory != NULL) {
			fCurrentAnimation = fAnimationFactory->AnimationFor(
										fAction,
										fActor->orientation, fColors);
		}
		fAnimationValid = true;
	} else if (fCurrentAnimation != NULL) {
		if (fAction != ACT_DEAD || !fCurrentAnimation->IsLastFrame())
			fCurrentAnimation->Next();
	}

	// TODO: Move to Actor::Update()
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
		_SetPositionPrivate(nextPoint);
	}		
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


void
Actor::UpdateTileCell()
{
	BackMap* backMap = dynamic_cast<AreaRoom*>(Core::Get()->CurrentRoom())->BackMap();
	if (backMap == NULL)
		return;

	::TileCell* oldTileCell = fTileCell;
	::TileCell* newTileCell = backMap->TileAtPoint(Position());

	if (oldTileCell != newTileCell) {
		if (oldTileCell != NULL)
			oldTileCell->RemoveObject(this);
		fTileCell = newTileCell;
		if (newTileCell != NULL)
			newTileCell->AddObject(this);
	}
}


void
Actor::SetTileCell(::TileCell* cell)
{
	fTileCell = cell;
}


void
Actor::_SetPositionPrivate(const IE::point& point)
{
	AreaRoom* room = dynamic_cast<AreaRoom*>(Core::Get()->CurrentRoom());
	if (room != NULL)
		room->SearchMap()->ClearPoint(fActor->position.x, fActor->position.y);

	fActor->position = point;

	if (room != NULL)
		room->SearchMap()->SetPoint(fActor->position.x, fActor->position.y);
}


void
Actor::_HandleColors()
{
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


void
Actor::_CheckRegion()
{
	return;
	Region* region = Core::Get()->RegionAtPoint(Position());
	if (region != NULL && region != fRegion) {
		/*trigger_entry entry;
		entry.name = "actor_over";
		entry.target = Name();
		region->AddTrigger(entry);*/
	}
	fRegion = region;
	if (fRegion != NULL)
		std::cout << Name() << " is over " << fRegion->Name() << std::endl;
}
