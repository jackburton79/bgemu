#include "Actor.h"

#include "2DAResource.h"
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
#include "ActionBar.h"
#include "Game.h"
#include "NPCRoster.h"
#include "GraphicsEngine.h"
#include "ITMResource.h"
#include "Log.h"
#include "Party.h"
#include "PathFind.h"
#include "Region.h"
#include "ResManager.h"
#include "SearchMap.h"
#include "SpellEffect.h"
#include "Script.h"
#include "TextSupport.h"
#include "TileCell.h"
#include "WedResource.h"

#include <algorithm>
#include <assert.h>

#include <string>


// Effective perception range, in area pixels. Matches the Infinity
// Engine's visual range: GemRB computes it as IE_VISUALRANGE (28) *
// VOODOO_CANSEE_F (16) = 448 for See()/Detect(), and cross-checks with a
// 28-search-cell radius (28 * 16px) for object matching. There it's a
// per-actor stat (SetVisualRange, effects); here it's fixed since this
// engine doesn't track IE_VISUALRANGE. Audible range for Shout()/Heard()
// is 3/2 of this (GemRB WithinAudibleRange: (3 * IE_VISUALRANGE) / 2).
// NB: distances here are Manhattan (IE::point_distance), not Euclidean
// like the original - a known approximation, not addressed in this pass.
static const int kVisualRange = 448;
static const int kAudibleRange = kVisualRange * 3 / 2;

// Fallback melee approach distance for NearestPoint() below, when the ARE
// actor's own "movement restriction distance" is 0 - which in practice is
// always, since real IESDP data attaches that field to a different, rare
// per-actor leash/wander setting, not combat range (the real engine derives
// how close to stand for an attack from both actors' AVATARS.2DA circle
// size instead, via GemRB's MoveNearerTo() - not modeled here). Without a
// real fallback, NearestPoint() returns the target's own exact position -
// which sits inside the target's own occupied search-map cell (actors keep
// their current cell blocked, see _SetPositionPrivate()), so every
// attacker's path to it fails outright and it never moves to melee range.
// Bigger than half a search-map cell (16x12px, see SearchMap.cpp) in
// either axis is enough to land the point in a different, unblocked cell.
static const int kMeleeApproachDistance = 16;


Actor::Actor(IE::actor &actor)
	:
	Object(actor.cre.CString(), Object::ACTOR),
	fActor(&actor),
	fAnimationFactory(NULL),
	fCurrentAnimation(NULL),
	fWeaponAnimation(NULL),
	fAnimationAction(ACT_STANDING),
	fNextAnimationAction(ACT_STANDING),
	fAnimationValid(false),
	fAnimationAutoSwitchOnEnd(false),
	fCRE(NULL),
	fOwnsActor(false),
	fColors(NULL),
	fFlying(false),
	fSelected(false),
	fAttackCooldown(0),
	fPath(NULL),
	fSpeed(2),
	fRegion(NULL),
	fOnWorldmapExit(false)
{
	_Init();
}


Actor::Actor(IE::actor &actor, CREResource* cre)
	:
	Object(actor.cre.CString(), Object::ACTOR),
	fActor(&actor),
	fAnimationFactory(NULL),
	fCurrentAnimation(NULL),
	fWeaponAnimation(NULL),
	fAnimationAction(ACT_STANDING),
	fNextAnimationAction(ACT_STANDING),
	fAnimationValid(false),
	fAnimationAutoSwitchOnEnd(false),
	fCRE(cre),
	fOwnsActor(false),
	fColors(NULL),
	fFlying(false),
	fSelected(false),
	fAttackCooldown(0),
	fPath(NULL),
	fSpeed(2),
	fRegion(NULL),
	fOnWorldmapExit(false)
{
	_Init();
}


Actor::Actor(const char* creName, IE::point position, int face)
	:
	Object(creName, Object::ACTOR),
	fActor(new IE::actor),
	fAnimationFactory(NULL),
	fCurrentAnimation(NULL),
	fWeaponAnimation(NULL),
	fAnimationAction(ACT_STANDING),
	fNextAnimationAction(ACT_STANDING),
	fAnimationValid(false),
	fAnimationAutoSwitchOnEnd(false),
	fCRE(NULL),
	fOwnsActor(true),
	fColors(NULL),
	fFlying(false),
	fSelected(false),
	fAttackCooldown(0),
	fPath(NULL),
	fSpeed(2),
	fRegion(NULL),
	fOnWorldmapExit(false),
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


// fActor->name is the ARE/spawn scripting name (a tag, e.g. "Anomen10" -
// the CRE resref for actors created via CreateCreature/the console, see
// the Actor(creName, ...) constructor), not a display name. The real
// display name lives in the CRE's own long-name strref; fall back to the
// scripting name only when there's no CRE data to read it from.
std::string
Actor::LongName() const
{
	if (fCRE != NULL) {
		std::string name = IDTable::GetDialog(fCRE->LongNameID());
		if (!name.empty())
			return name;
	}
	return fActor->name;
}


void
Actor::SetLongName(const char* name)
{
	if (name == NULL)
		return;
	strncpy(fActor->name, name, sizeof(fActor->name) - 1);
	fActor->name[sizeof(fActor->name) - 1] = '\0';
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
	}

	// Also for an actor handed its CRE (placed in a checkpointed area, or
	// otherwise built from saved CRE data): without this it kept a NULL
	// color set and drew the BAM's raw placeholder palette.
	_HandleColors();

	// This only makes sense for actors already created once
	if (fCRE->GlobalActorEnum() != uint16(-1))
		SetGlobalID(fCRE->GlobalActorEnum());

	// TODO: Get all scripts ? or just the specific one ?

	fAnimationFactory = AnimationFactory::GetFactory(fCRE->AnimationID());

	// TODO: Are we overwriting the actor specific stuff here ?
	fActor->script_override = fCRE->OverrideScriptName();
	fActor->script_class = fCRE->ClassScriptName();
	fActor->script_race = fCRE->RaceScriptName();
	fActor->script_default = fCRE->DefaultScriptName();
	fActor->script_general = fCRE->GeneralScriptName();

	_HandleScripts();

	SetActive(true);

	// A freshly created character (CharacterBuilder / StartingParty)
	// comes in at class level 0 with placeholder HP/THAC0/saves; run the
	// level-up path once to fill in the real level-1 values from the
	// class tables. Real placed CREs are already level >= 1 and skip this.
	if (fCRE != NULL && fCRE->ClassLevel(0) == 0)
		_CheckLevelUp();

	// TODO: Check if it's okay. It's here because it seems it could be uninitialized
	fActor->destination = fActor->position;

	if (fCRE->PermanentStatus() == STATE_DEAD)
		SetAnimationAction(ACT_DEAD);
	else
		SetAnimationAction(ACT_STANDING);
}


Actor::~Actor()
{
	// TODO: Since actions keep a reference to actor,
	// this won't be ever called if an actor has actions in the list,
	// and if it doesn't, there is no point to call it here
	ClearActionList();

	if (fOwnsActor)
		delete fActor;

	gResManager->ReleaseResource(fCRE);

	delete fColors;

	if (fAnimationFactory != NULL) {
		AnimationFactory::ReleaseFactory(fAnimationFactory);
		fAnimationFactory = NULL;
	}

	if (fNameBitmap != NULL)
		fNameBitmap->Release();
	delete fCurrentAnimation;
	delete fWeaponAnimation;
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
	std::cout << "Hitpoints: " << std::dec << cre->CurrentHitPoints()
		<< "/" << cre->MaxHitPoints() << std::endl;
	std::cout << "Levels: " << (int)cre->ClassLevel(0) << "/"
		<< (int)cre->ClassLevel(1) << "/" << (int)cre->ClassLevel(2) << std::endl;
	std::cout << "THAC0: " << (int)cre->THAC0() << std::endl;
	SaveVersus sv = cre->Saves();
	std::cout << "Saves (death/wands/poly/breath/spell): "
		<< (int)sv.death << "/" << (int)sv.wands << "/" << (int)sv.poly
		<< "/" << (int)sv.breath << "/" << (int)sv.spell << std::endl;
	if (cre->OpenLocksSkill() > 0 || cre->FindTrapsSkill() > 0) {
		std::cout << "Open Locks: " << (int)cre->OpenLocksSkill()
			<< "  Find Traps: " << (int)cre->FindTrapsSkill() << std::endl;
	}
	{
		std::vector<cre_known_spell> known = cre->KnownSpells();
		std::vector<cre_memorized_spell> memo = cre->MemorizedSpells();
		if (!known.empty() || !memo.empty()) {
			std::cout << "Known spells:";
			for (const cre_known_spell& s : known)
				std::cout << " " << s.spell.CString();
			std::cout << std::endl << "Memorized:";
			for (const cre_memorized_spell& s : memo)
				std::cout << " " << s.spell.CString()
					<< ((s.flags & 1) ? "" : "(used)");
			std::cout << std::endl;
		}
	}
	std::cout << "Status flags: " << std::dec << cre->PermanentStatus() << std::endl;
	std::cout << "Reputation: " << (int)cre->Reputation() << std::endl;
	std::cout << "Morale: " << (int)cre->Morale() << std::endl;
	std::cout << "Gold: " << cre->Gold() << std::endl;
	std::cout << "Selected: " << (IsSelected() ? "yes" : "no") << std::endl;
	fActor->Print();
	std::cout << "*********" << std::endl;
}


const ::Bitmap*
Actor::Bitmap() const
{
	if (fCurrentAnimation == NULL) {
		std::string message("Actor::Bitmap() (");
		message.append(fCRE->Name()).append(") : No current animation!");
		return nullptr;
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
Actor::SetPosition(const IE::point& position, bool triggerTravel)
{
	_SetPositionPrivate(position, triggerTravel);

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
	_SetOrientation(toPoint);
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
	if (fPath == NULL)
		fPath = new Path(); //(PathFinder::kStep, AreaRoom::IsPointPassable);

	IE::point destination = fActor->position;

	test_function func;
	if (ignoreSearchMap)
		func = Actor::PointPassableTrue;
	else
		func = AreaRoom::IsPointPassable;

	// The actor's own currently occupied search-map cell is marked
	// impassable (so other actors don't walk into it) - but the search
	// is fine-grained (a few pixels per step) and can't cross a whole
	// blocked cell in one hop, so leaving it blocked here can trap the
	// actor unable to path away from its own position (how "stuck" this
	// makes it depends on where exactly it sits within the cell, which
	// is why it doesn't happen on every move). Clear it for the duration
	// of the search, then restore it right after - same Clear/Set
	// bracketing _SetPositionPrivate() already uses for real movement.
	AreaRoom* room = Area();
	SearchMap* searchMap = room != NULL ? room->SearchMap() : NULL;
	if (searchMap != NULL)
		searchMap->ClearPoint(fActor->position.x, fActor->position.y);

	try {
		fPath->Set(fActor->position, point, func);
		destination = fPath->End();
	} catch (...) {
		std::cerr << Log::Red << Name() << ": Actor::SetDestination() failed!" << Log::Normal << std::endl;
	}

	if (searchMap != NULL)
		searchMap->SetPoint(fActor->position.x, fActor->position.y);

	fActor->destination = destination;
}


void
Actor::ClearDestination()
{
	fActor->destination = fActor->position;
}


IE::point
Actor::HomeLocation() const
{
	return fHomeLocation;
}


void
Actor::SetHomeLocation(const IE::point& position)
{
	fHomeLocation = position;
}


void
Actor::Draw(AreaRoom* room) const
{
	if (CRE()->PermanentStatus() != ACT_DEAD)
		_DrawCircle(room);

	IE::point actorPosition = Position();
	actorPosition.y += room->PointHeight(actorPosition) - 8;
	room->DrawBitmap(Bitmap(), actorPosition, true);
	if (fWeaponAnimation != NULL)
		room->DrawBitmap(fWeaponAnimation->Bitmap(), actorPosition, true);

	if (InParty())
		_DrawActorPath(room);
	_DrawActorText(room);
	_DrawActorName(room);
}


void
Actor::_DrawActorText(AreaRoom* room) const
{
	std::string text = Text();
	if (!text.empty()) {
		const Font* font = FontRoster::GetFont("TOOLFONT");
		// TODO: Change text color based on actor
		::Bitmap* bitmap = font->GetRenderedString(text, 0, GFX::kPaletteYellow);
		IE::point textPoint = Position();
		textPoint.y -= 100;
		room->DrawBitmap(bitmap, textPoint, false);
		bitmap->Release();
	}
}


void
Actor::_DrawActorName(AreaRoom* room) const
{
	const uint32 strRef = fCRE != NULL ? fCRE->LongNameID() : 0;
	if (fNameBitmap == NULL || strRef != fNameBitmapStrRef || fNameBitmapName != Name()
			|| fNameBitmapLongName != fActor->name) {
		if (fNameBitmap != NULL)
			fNameBitmap->Release();
		fNameBitmap = NULL;
		std::string text = LongName();
		text.append(" (");
		text.append(Name()).append(")");
		const Font* font = FontRoster::GetFont("TOOLFONT");
		fNameBitmap = font->GetRenderedString(text, 0, GFX::kPaletteYellow);
		fNameBitmapStrRef = strRef;
		fNameBitmapName = Name();
		fNameBitmapLongName = fActor->name;
	}

	IE::point textPoint = Position();
	textPoint.y += 30;
	room->DrawBitmap(fNameBitmap, textPoint, false);
}


void
Actor::_DrawActorPath(AreaRoom* room) const
{
	/*std::vector<IE::point> points;
	if (!fPath)
		return;
	fPath->GetPoints(points);
	::Bitmap* image = room->BackMap()->Image();
	if (image->Lock()) {
		for (std::vector<IE::point>::iterator i = points.begin(); i != points.end(); i++) {
			IE::point point = *i;
			room->ConvertFromArea(point);
			image->StrokeCircle(point.x, point.y, 3, image->MapColor(0, 240, 0));
		}
		image->Unlock();
	}*/
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
			color = image->MapRGBColor(0, 255, 0);
		else
			color = image->MapRGBColor(255, 0, 0);

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
	IE::point restriction = RestrictionDistance();
	if (restriction.x == 0)
		restriction.x = kMeleeApproachDistance;
	if (restriction.y == 0)
		restriction.y = kMeleeApproachDistance;
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


void
Actor::ApplyDamage(int32 amount)
{
	if (amount <= 0)
		return;

	CREResource* cre = CRE();
	int32 hp = (int32)cre->CurrentHitPoints() - amount;
	cre->SetCurrentHitPoints((uint16)std::max(hp, 0));

	// TookDamage() - no target, just records that this happened this round.
	trigger_entry tookDamage("TookDamage");
	tookDamage.round = Core::Get()->ScriptRound();
	AddTrigger(tookDamage);

	if (hp > 0 || IsState(STATE_DEAD)) // already dead, nothing to do
		return;

	cre->SetPermanentStatus(cre->PermanentStatus() | STATE_DEAD);
	SetAnimationAction(ACT_DIE); // auto-chains to ACT_DEAD once it finishes

	// Died() - same round-stamped pattern as AttackedBy/TookDamage above.
	trigger_entry died("Died");
	died.round = Core::Get()->ScriptRound();
	AddTrigger(died);

	// Award this creature's kill XP (CRE offset 0x14) to the party, same
	// as the real engine does at the moment of death - not gated on who
	// actually landed the killing blow (unlike the real engine's more
	// elaborate IF_GIVEXP bookkeeping), so a monster killed by another
	// hostile, by a trap, etc. still pays out; deliberately-deferred
	// approximation until an attacker is threaded through ApplyDamage().
	if (!InParty()) {
		uint32 xpValue = cre->ExperienceValue();
		if (xpValue > 0)
			Game::Get()->Party()->ShareExperience(xpValue);
	}

	// Drop whatever was queued and clear destination
	ClearActionList();
	ClearDestination();

	// Corpses stay in the area - no DestroySelf() here
	const std::string deathVar = cre->DeathVariable();
	if (!deathVar.empty())
		SetVariable(deathVar.c_str(), 1);
}


bool
Actor::IsState(int state) const
{
	if (CRE()->PermanentStatus() & state)
		return true;

	return false;
}


void
Actor::GainExperience(uint32 amount)
{
	if (amount == 0)
		return;

	CREResource* cre = CRE();
	cre->SetExperience(cre->Experience() + amount);
	_CheckLevelUp();
}


// Maps a single class name (one "_"-separated component of CRE()->Class()'s
// CLASS.IDS name, e.g. "FIGHTER" out of "FIGHTER_THIEF") to the 2DA
// resources that drive its level progression - see IESDP's hpx.2da/
// thac0.2da/savexxx.2da docs. Classes not listed here (any monster
// "class", or the race-based save variants for dwarves/gnomes/halflings)
// simply don't level up through this path.
struct ClassProgression {
	const char* name;
	const char* hpTable;
	const char* saveTable;
	bool warrior; // true: HPCONBON's WARRIOR bonus column, else OTHER
};

static const ClassProgression kClassProgressions[] = {
	{ "FIGHTER",  "HPWAR",  "SAVEWAR",  true },
	{ "PALADIN",  "HPWAR",  "SAVEWAR",  true },
	{ "RANGER",   "HPWAR",  "SAVEWAR",  true },
	{ "MAGE",     "HPWIZ",  "SAVEWIZ",  false },
	{ "SORCERER", "HPWIZ",  "SAVEWIZ",  false },
	{ "CLERIC",   "HPPRS",  "SAVEPRS",  false },
	{ "DRUID",    "HPPRS",  "SAVEPRS",  false },
	{ "THIEF",    "HPROG",  "SAVEROG",  false },
	{ "BARD",     "HPROG",  "SAVEROG",  false },
	{ "MONK",     "HPMONK", "SAVEMONK", false },
};


static const ClassProgression*
_ProgressionFor(const std::string& className)
{
	for (const ClassProgression& progression : kClassProgressions) {
		if (className == progression.name)
			return &progression;
	}
	return NULL;
}


// TWODAResource::ValueFor()/IntegerValueFor() throw std::out_of_range for
// any (row, column) pair the file doesn't actually list (e.g. a level
// beyond the table's last defined column, or a class name that isn't one
// of its rows) - it does NOT fall back to the file's declared default
// value. This wraps every lookup so a missing entry just yields `fallback`
// instead of crashing the whole engine.
static int32
_TableValue(TWODAResource* table, const char* row, const char* column, int32 fallback)
{
	if (table == NULL)
		return fallback;
	try {
		return table->IntegerValueFor(row, column);
	} catch (const std::exception&) {
		return fallback;
	}
}


// Applies XP-driven level-up(s) for every "_"-separated class component of
// this creature's Class() (up to 3, in the same slot order CREResource
// uses for bytes 0x234/0x235/0x236 - see CREResource::ClassLevel()'s
// comment). For each component with real progression data (see
// kClassProgressions above), rolls new hit points for every level gained
// (HPxxx.2da + HPCONBON.2da) and recomputes THAC0/saving throws as the
// best (lowest) value among all of this creature's classes at their
// (possibly just-updated) level - matching how AD&D 2E multi-classing
// works. NumberOfAttacks() and spellbook memorization slots are not
// recomputed here - both need extra infrastructure this pass doesn't add
// (see the Fase 7 plan notes).
void
Actor::_CheckLevelUp()
{
	CREResource* cre = CRE();
	const uint32 xp = cre->Experience();

	std::string className = IDTable::ClassAt(cre->Class());
	std::vector<std::string> classTokens;
	size_t start = 0;
	for (size_t i = 0; i <= className.size(); i++) {
		if (i == className.size() || className[i] == '_') {
			classTokens.push_back(className.substr(start, i - start));
			start = i + 1;
		}
	}
	if (classTokens.empty() || classTokens.size() > 3)
		return; // not a recognizable CLASS.IDS class name

	TWODAResource* xpLevel = gResManager->Get2DA("XPLEVEL");
	if (xpLevel == NULL)
		return;

	TWODAResource* hpConBon = gResManager->Get2DA("HPCONBON");
	TWODAResource* thac0Table = gResManager->Get2DA("THAC0");

	BaseAttributes attributes;
	cre->GetAttributes(attributes);
	char conRow[8];
	snprintf(conRow, sizeof(conRow), "%d", (int)attributes.constitution);

	bool leveledUp = false;
	uint16 hpGain = 0;
	uint8 bestThac0 = 255;
	SaveVersus bestSaves = { 255, 255, 255, 255, 255 };

	for (size_t slot = 0; slot < classTokens.size(); slot++) {
		const ClassProgression* progression = _ProgressionFor(classTokens[slot]);
		if (progression == NULL)
			continue;
		const char* token = classTokens[slot].c_str();

		const uint8 currentLevel = cre->ClassLevel((uint8)slot);
		uint8 newLevel = currentLevel;
		for (uint8 level = currentLevel + 1; level <= 41; level++) {
			char column[8];
			snprintf(column, sizeof(column), "%u", level);
			int32 threshold = _TableValue(xpLevel, token, column, -1);
			if (threshold < 0 || (uint32)threshold > xp)
				break;
			newLevel = level;
		}

		char levelColumn[8];
		snprintf(levelColumn, sizeof(levelColumn), "%u", std::max<uint8>(newLevel, 1));

		int32 thac0 = _TableValue(thac0Table, token, levelColumn, -1);
		if (thac0 >= 0 && (uint8)thac0 < bestThac0)
			bestThac0 = (uint8)thac0;

		TWODAResource* saveTable = gResManager->Get2DA(progression->saveTable);
		if (saveTable != NULL) {
			int32 death = _TableValue(saveTable, "DEATH", levelColumn, -1);
			int32 wands = _TableValue(saveTable, "WANDS", levelColumn, -1);
			int32 poly = _TableValue(saveTable, "POLY", levelColumn, -1);
			int32 breath = _TableValue(saveTable, "BREATH", levelColumn, -1);
			int32 spell = _TableValue(saveTable, "SPELL", levelColumn, -1);
			if (death >= 0 && (uint8)death < bestSaves.death)
				bestSaves.death = (uint8)death;
			if (wands >= 0 && (uint8)wands < bestSaves.wands)
				bestSaves.wands = (uint8)wands;
			if (poly >= 0 && (uint8)poly < bestSaves.poly)
				bestSaves.poly = (uint8)poly;
			if (breath >= 0 && (uint8)breath < bestSaves.breath)
				bestSaves.breath = (uint8)breath;
			if (spell >= 0 && (uint8)spell < bestSaves.spell)
				bestSaves.spell = (uint8)spell;
			gResManager->ReleaseResource(saveTable);
		}

		if (newLevel <= currentLevel)
			continue;

		leveledUp = true;
		cre->SetClassLevel((uint8)slot, newLevel);

		TWODAResource* hpTable = gResManager->Get2DA(progression->hpTable);
		if (hpTable != NULL) {
			for (uint8 level = currentLevel + 1; level <= newLevel; level++) {
				char hpRow[8];
				snprintf(hpRow, sizeof(hpRow), "%u", level);
				int32 sides = _TableValue(hpTable, hpRow, "SIDES", 0);
				int32 rolls = _TableValue(hpTable, hpRow, "ROLLS", 0);
				int32 modifier = _TableValue(hpTable, hpRow, "MODIFIER", 0);
				if (sides > 0 && rolls > 0) {
					// Level 1 (character creation, currentLevel == 0)
					// grants maximum hit points, as the original game
					// does; subsequent levels are rolled.
					hpGain += (currentLevel == 0)
						? (uint16)(rolls * sides)
						: Core::RollDice(rolls, sides, 0);
				}
				hpGain += modifier;
				hpGain += _TableValue(hpConBon, conRow,
					progression->warrior ? "WARRIOR" : "OTHER", 0);
			}
			gResManager->ReleaseResource(hpTable);
		}
	}

	if (hpConBon != NULL)
		gResManager->ReleaseResource(hpConBon);
	if (thac0Table != NULL)
		gResManager->ReleaseResource(thac0Table);
	gResManager->ReleaseResource(xpLevel);

	if (!leveledUp)
		return;

	if (hpGain > 0) {
		cre->SetMaxHitPoints(cre->MaxHitPoints() + hpGain);
		cre->SetCurrentHitPoints(cre->CurrentHitPoints() + hpGain);
	}
	if (bestThac0 != 255)
		cre->SetTHAC0(bestThac0);
	if (bestSaves.death != 255) {
		SaveVersus saves = cre->Saves();
		if (bestSaves.death < saves.death)
			saves.death = bestSaves.death;
		if (bestSaves.wands < saves.wands)
			saves.wands = bestSaves.wands;
		if (bestSaves.poly < saves.poly)
			saves.poly = bestSaves.poly;
		if (bestSaves.breath < saves.breath)
			saves.breath = bestSaves.breath;
		if (bestSaves.spell < saves.spell)
			saves.spell = bestSaves.spell;
		cre->SetSaves(saves);
	}
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


// Slot numbers below are indices into the CRE's own 40-slot item array -
// NOT SLOTS.IDS values (that table numbers slots differently and is only
// used by scripts/UI, not by the on-disk array order). Confirmed
// empirically against a real BG2 companion CRE (ANOMEN10): helm/armor/
// shield/weapon items landed at indices 0/1/2/9, matching the array order
// IESDP cre_v1 documents (Helmet, Armor, Shield, Gloves, L.Ring, R.Ring,
// Amulet, Belt, Boots, Weapon1-4, Quiver1-4, Cloak, QuickItem1-3,
// Inventory1-16, MagicWeapon, SelectedWeapon, SelectedWeaponAbility).
// (kSlot* constants now live in CreResource.h, shared with the inventory
// GUI, which needs to map its own control IDs to these same slots.)


// Maps an ITM "Item type" (IESDP itm_v1) to the slot it equips into by
// default. Returns -1 for types that are never equipped (misc, potions,
// scrolls, food, keys, books) - those only ever live in a general
// inventory slot.
static int32
_DefaultSlotForItemType(uint16 type)
{
	switch (type) {
		case 0x0001: return kSlotAmulet;
		case 0x0002: return kSlotArmor;
		case 0x0003: return kSlotBelt;
		case 0x0004: return kSlotBoots;
		case 0x0006: return kSlotGauntlets;
		case 0x0007: return kSlotHelmet;
		case 0x000a: return kSlotRingLeft;
		case 0x000c: return kSlotShield;
		case 0x0020: return kSlotCloak; // BG2 cloak/robe item type
		case 0x0005: // Arrows
		case 0x000e: // Bullets
			return kSlotAmmoFirst;
		case 0x0000: // Books/misc
		case 0x0008: // Keys
		case 0x0009: // Potions
		case 0x000b: // Scrolls
		case 0x000d: // Food
			return -1;
		default:
			// Everything else in the ITM type table is a weapon
			// (daggers, swords, axes, bows, staves, etc).
			return kSlotWeaponFirst;
	}
}


// Whether an item of this ITM type is allowed to sit in `slot`. General
// inventory and quick-item slots take anything; the equipment slots each
// take their own type (weapon slots take any weapon, quiver slots any
// ammo, both ring slots any ring).
static bool
_SlotAcceptsItemType(uint32 slot, uint16 itemType)
{
	if (slot >= kSlotGeneralFirst && slot <= kSlotGeneralLast)
		return true;
	if (slot >= 18 && slot <= 20) // QuickItem1-3
		return true;
	if (slot >= kSlotWeaponFirst && slot < kSlotWeaponFirst + 4)
		return _DefaultSlotForItemType(itemType) == (int32)kSlotWeaponFirst;
	if (slot >= kSlotAmmoFirst && slot <= kSlotAmmoLast)
		return _DefaultSlotForItemType(itemType) == (int32)kSlotAmmoFirst;
	if (slot == kSlotRingLeft || slot == kSlotRingLeft + 1)
		return itemType == 0x000a;
	return _DefaultSlotForItemType(itemType) == (int32)slot;
}


// ITM type of a slot's current occupant, or 0 (a valid "misc" type, but
// also the harmless fallback) if the slot is empty or the item resource
// can't be loaded.
static uint16
_ItemTypeAtSlot(CREResource* cre, uint32 slot)
{
	IE::item item;
	if (!cre->GetItemAtSlot(slot, item))
		return 0;
	ITMResource* itm = gResManager->GetITM(item.name);
	if (itm == NULL)
		return 0;
	uint16 type = itm->ItemType();
	gResManager->ReleaseResource(itm);
	return type;
}


std::string
Actor::ArmorAnimation() const
{
	// TODO: Refactor: items should be loaded elsewhere
	IE::item armor;
	if (fCRE->GetItemAtSlot(kSlotArmor, armor)) {
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
Actor::PaperdollName() const
{
	if (fAnimationFactory == NULL)
		return "";
	return fAnimationFactory->PaperdollName(this);
}


std::string
Actor::WeaponAnimation() const
{
	ITMResource* itm = EquippedWeapon();
	if (itm == NULL)
		return "";

	std::string animationString = itm->Animation();
	gResManager->ReleaseResource(itm);
	return animationString;
}


static int32 _FindAmmoSlot(CREResource* cre, uint8 mask);


WeaponAnimationType
Actor::EquippedWeaponAnimationType() const
{
	WeaponAnimationType type;
	ITMResource* itm = EquippedWeapon();
	if (itm == NULL)
		return type; // unarmed - one-handed melee

	itm_ability ability;
	if (itm->GetAbility(0, ability) && ability.attackType == 4
			&& _FindAmmoSlot(fCRE, ability.projectileQualifier) >= 0) { // Launcher
		// A bow/sling/crossbow's own header often carries the "Two-handed"
		// flag too (lore-accurate), but the Shooting sequence has no 2H
		// variant - a launcher is one-handed from an animation POV.
		type.isRanged = true;
		type.isCrossbow = ability.crossbowQualifier != 0;
	} else
		type.isTwoHanded = itm->IsTwoHanded();

	gResManager->ReleaseResource(itm);
	return type;
}


// Whether the item in `slot` is a launcher or ammunition carrying one of
// the projectile qualifier bits in `mask`; `launcher` picks which of the
// two the slot must be (a bow's ability 0 is attack type 4, an arrow's 2).
static bool
_SlotHasProjectile(CREResource* cre, uint32 slot, uint8 mask, bool launcher)
{
	IE::item item;
	if (!cre->GetItemAtSlot(slot, item) || item.name.name[0] == '\0')
		return false;

	ITMResource* itm = gResManager->GetITM(item.name);
	if (itm == NULL)
		return false;
	itm_ability ability;
	const bool found = itm->GetAbility(0, ability);
	gResManager->ReleaseResource(itm);
	return found && (ability.attackType == 4) == launcher
		&& (ability.projectileQualifier & mask) != 0;
}


// First quiver slot holding ammunition for a launcher that fires `mask`.
static int32
_FindAmmoSlot(CREResource* cre, uint8 mask)
{
	for (uint32 slot = kSlotAmmoFirst; slot <= kSlotAmmoLast; slot++) {
		if (_SlotHasProjectile(cre, slot, mask, false))
			return (int32)slot;
	}
	return -1;
}


int32
Actor::ActiveWeaponSlot() const
{
	const uint16 code = fCRE->SelectedWeaponCode();
	if (code == kSelectedWeaponFists)
		return -1;

	int32 slot = kSlotWeaponFirst;
	if (code < kNumWeaponSlots) {
		slot += code;
	} else if (kSlotWeaponFirst + code <= kSlotAmmoLast) {
		// A launcher is selected through its ammunition: find the bow
		// that fires it.
		IE::item ammo;
		ITMResource* itm = NULL;
		if (fCRE->GetItemAtSlot(kSlotWeaponFirst + code, ammo))
			itm = gResManager->GetITM(ammo.name);
		itm_ability ability;
		const bool found = itm != NULL && itm->GetAbility(0, ability);
		if (itm != NULL)
			gResManager->ReleaseResource(itm);
		if (!found)
			return -1;
		slot = -1;
		for (uint32 i = 0; i < kNumWeaponSlots; i++) {
			if (_SlotHasProjectile(fCRE, kSlotWeaponFirst + i,
					ability.projectileQualifier, true)) {
				slot = kSlotWeaponFirst + i;
				break;
			}
		}
	}
	// Any other value is not a code the games write: read it as the first
	// quickslot rather than disarming the creature.

	if (slot < 0 || fCRE->ItemsIndexAtSlot((uint32)slot) < 0)
		return -1;
	return slot;
}


bool
Actor::SelectWeapon(int32 index)
{
	if (index >= (int32)kNumWeaponSlots || index < -1)
		return false;
	fCRE->SetSelectedWeaponCode(index < 0 ? (uint16)kSelectedWeaponFists : (uint16)index);
	InvalidateAnimation(); // the weapon layer changes
	if (InParty())
		Game::Get()->Bar().Refresh();
	return true;
}


ITMResource*
Actor::EquippedWeapon() const
{
	IE::item weapon;
	const int32 slot = ActiveWeaponSlot();
	if (slot < 0 || !fCRE->GetItemAtSlot((uint32)slot, weapon))
		return NULL;

	return gResManager->GetITM(weapon.name);
}


// Unarmed, or the equipped item has no usable ability: there's no
// dedicated "fists" ITM resource to load, so fall back to a small
// hardcoded unarmed profile instead.
static attack_profile
_UnarmedProfile()
{
	attack_profile profile;
	profile.ability.attackType = 1; // Melee
	profile.ability.thac0Bonus = 0;
	profile.ability.diceSides = 2;
	profile.ability.diceThrown = 1;
	profile.ability.damageBonus = 0;
	profile.ability.damageType = 5; // Fists
	return profile;
}


attack_profile
Actor::AttackProfile() const
{
	attack_profile profile;

	ITMResource* weapon = EquippedWeapon();
	itm_ability ability;
	const bool hasAbility = weapon != NULL && weapon->GetAbility(0, ability);
	std::vector<spl_effect> weaponEffects;
	if (weapon != NULL) {
		if (hasAbility)
			weaponEffects = weapon->OnHitEffects(0);
		gResManager->ReleaseResource(weapon);
	}

	if (!hasAbility)
		return _UnarmedProfile();

	profile.ability = ability;
	profile.onHitEffects = weaponEffects;
	if (ability.attackType == 2) {
		// Thrown weapon: the stack in the weapon slot is the ammunition.
		profile.ranged = true;
		profile.rangeFeet = ability.range;
		profile.spentSlot = ActiveWeaponSlot();
	} else if (ability.attackType == 4) {
		const int32 ammoSlot = _FindAmmoSlot(fCRE, ability.projectileQualifier);
		if (ammoSlot < 0)
			return _UnarmedProfile();
		// The launcher supplies range and its own bonuses; the ammunition
		// supplies the dice and damage type (same split GemRB makes).
		IE::item ammo;
		fCRE->GetItemAtSlot((uint32)ammoSlot, ammo);
		ITMResource* ammoItm = gResManager->GetITM(ammo.name);
		itm_ability ammoAbility;
		const bool ammoOk = ammoItm != NULL && ammoItm->GetAbility(0, ammoAbility);
		if (ammoOk) {
			for (const spl_effect& effect : ammoItm->OnHitEffects(0))
				profile.onHitEffects.push_back(effect);
		}
		if (ammoItm != NULL)
			gResManager->ReleaseResource(ammoItm);
		if (!ammoOk)
			return _UnarmedProfile();
		profile.ability.diceSides = ammoAbility.diceSides;
		profile.ability.diceThrown = ammoAbility.diceThrown;
		profile.ability.damageType = ammoAbility.damageType;
		profile.ability.thac0Bonus = ability.thac0Bonus + ammoAbility.thac0Bonus;
		profile.ability.damageBonus = ability.damageBonus + ammoAbility.damageBonus;
		profile.ranged = true;
		profile.rangeFeet = ability.range;
		profile.spentSlot = ammoSlot;
	}
	return profile;
}


/* static */
int32
Actor::StrengthBonus(CREResource* cre, int column)
{
	BaseAttributes attrs;
	cre->GetAttributes(attrs);

	int32 bonus = 0;
	TWODAResource* strmod = gResManager->Get2DA("STRMOD");
	if (strmod != NULL) {
		bonus = strmod->IntegerValueAt(attrs.strength, column);
		gResManager->ReleaseResource(strmod);
	}
	if (attrs.strength == 18 && attrs.strength_bonus > 0) {
		TWODAResource* strmodex = gResManager->Get2DA("STRMODEX");
		if (strmodex != NULL) {
			bonus += strmodex->IntegerValueAt(attrs.strength_bonus, column);
			gResManager->ReleaseResource(strmodex);
		}
	}
	return bonus;
}


void
Actor::CastSpell(const res_ref& spell, Actor* target)
{
	// SPELL.IDS numbers a resref by its class digit (SPPR 1, SPWI 2, SPIN 3,
	// SPCL 4) followed by the rest of the name: SPWI304 is 2304.
	const std::string name = spell.CString();
	if (target == NULL || name.size() < 5)
		return;
	static const char* const kPrefixes[] = { "SPPR", "SPWI", "SPIN", "SPCL" };
	int digit = 0;
	for (int i = 0; i < 4; i++) {
		if (name.compare(0, 4, kPrefixes[i]) == 0)
			digit = i + 1;
	}
	if (digit == 0)
		return;

	ClearActionList();
	action_params* params = new action_params(Name(), target->Name());
	params->Second()->globalId = target->GlobalID();
	params->id = 31; // SPELL
	params->integer1 = digit * 1000 + atoi(name.c_str() + 4);
	AddAction(params);
	params->Release();
}


res_ref
Actor::QuickSpell(uint32 index) const
{
	return index < kNumQuickSpells ? fQuickSpells[index] : res_ref();
}


void
Actor::SetQuickSpell(uint32 index, const res_ref& spell)
{
	if (index < kNumQuickSpells)
		fQuickSpells[index] = spell;
}


void
Actor::UseItem(uint32 slot, Actor* target)
{
	if (target == NULL)
		return;
	ClearActionList();
	action_params* params = new action_params(Name(), target->Name());
	params->Second()->globalId = target->GlobalID();
	params->id = 34; // USEITEMSLOT
	params->integer1 = (int32)slot;
	AddAction(params);
	params->Release();
}


void
Actor::ConsumeFromSlot(uint32 slot)
{
	IE::item item;
	const int32 itemsIndex = fCRE->ItemsIndexAtSlot(slot);
	if (itemsIndex < 0 || !fCRE->GetItemAtSlot(slot, item))
		return;

	if (item.quantity1 > 1) {
		item.quantity1--;
		fCRE->SetItemAtItemsIndex((uint16)itemsIndex, item);
		return;
	}
	_ClearItemSlot(slot);
	InvalidateAnimation();
	if (InParty())
		Game::Get()->Bar().Refresh();
}


// Real resref both BG1 and BG2 use for a gold-pile item - see this
// method's own declaration comment. Not something either game's real
// data exposes as a lookup table (GemRB's own "randitem.2da", which
// names it too, is a GemRB-authored table, not real IE game data);
// just a well-known fixed resref in both games' actual MISC07.ITM.
static const res_ref kGoldItemResRef = "MISC07";


bool
Actor::AddItem(const res_ref& itemName, uint16 quantity)
{
	IE::item item;
	item.name = itemName;
	item.expiration_time = 0;
	item.expiration_time2 = 0;
	item.quantity1 = quantity;
	item.quantity2 = 0;
	item.quantity3 = 0;
	// No flags: an item that needs identifying starts out unidentified, as
	// the real engine's CreateItem does (Store::SlotFlags() still treats one
	// with no lore to identify as identified).
	item.flags = 0;
	return AddItem(item);
}


bool
Actor::AddItem(const IE::item& newItem)
{
	if (newItem.name == kGoldItemResRef) {
		Core::Get()->AddPartyGold(newItem.quantity1 > 0 ? newItem.quantity1 : 1);
		return true;
	}

	const res_ref& itemName = newItem.name;
	ITMResource* itm = gResManager->GetITM(itemName);
	if (itm == NULL) {
		std::cerr << Name() << ": AddItem(" << itemName.CString()
				<< "): no such item resource" << std::endl;
		return false;
	}
	gResManager->ReleaseResource(itm);

	int32 slot = fCRE->FindFreeSlot(kSlotGeneralFirst, kSlotGeneralLast);
	if (slot < 0) {
		std::cerr << Name() << ": AddItem(" << itemName.CString()
				<< "): no free inventory slot" << std::endl;
		return false;
	}

	int32 itemsIndex = fCRE->AllocItemsEntry();
	if (itemsIndex < 0) {
		std::cerr << Name() << ": AddItem(" << itemName.CString()
				<< "): no free Items table entry" << std::endl;
		return false;
	}

	IE::item item = newItem;
	if (item.quantity1 == 0)
		item.quantity1 = 1;

	fCRE->SetItemAtItemsIndex((uint16)itemsIndex, item);
	fCRE->SetItemAtSlot((uint32)slot, itemsIndex);
	if (InParty())
		Game::Get()->Bar().Refresh(); // Use may have something to offer now
	return true;
}


bool
Actor::IdentifyItemInSlot(uint32 slot)
{
	IE::item item;
	int32 itemsIndex = fCRE->ItemsIndexAtSlot(slot);
	if (itemsIndex < 0 || !fCRE->GetItemAtSlot(slot, item))
		return false;
	item.flags |= 1; // Identified
	fCRE->SetItemAtItemsIndex((uint16)itemsIndex, item);
	return true;
}


// Zeroes the Items-table entry backing `slot` (so FindFreeItemsEntry()
// can reuse it) and unlinks the slot. Shared by RemoveItem() and
// TakeItemFromSlot().
void
Actor::_ClearItemSlot(uint32 slot)
{
	int32 itemsIndex = fCRE->ItemsIndexAtSlot(slot);
	if (itemsIndex < 0)
		return;
	IE::item empty;
	empty.name = res_ref();
	empty.expiration_time = 0;
	empty.expiration_time2 = 0;
	empty.quantity1 = 0;
	empty.quantity2 = 0;
	empty.quantity3 = 0;
	empty.flags = 0;
	fCRE->SetItemAtItemsIndex((uint16)itemsIndex, empty);
	fCRE->SetItemAtSlot(slot, -1);
	if (InParty())
		Game::Get()->Bar().Refresh();
}


bool
Actor::RemoveItem(const res_ref& itemName)
{
	int32 slot = fCRE->FindItemSlot(itemName);
	if (slot < 0)
		return false;

	_ClearItemSlot((uint32)slot);
	return true;
}


// Removes whatever occupies `slot`, handing back its full IE::item (name
// + quantities) in `out` - used to move an item out of the inventory and
// onto the ground (Game::DropHeldItemOnGround()). Returns false for an
// empty/invalid slot.
bool
Actor::TakeItemFromSlot(uint32 slot, IE::item& out)
{
	if (fCRE == NULL || slot >= kNumItemSlots)
		return false;
	if (!fCRE->GetItemAtSlot(slot, out) || out.name.name[0] == '\0')
		return false;

	_ClearItemSlot(slot);
	if (slot < kSlotGeneralFirst) {
		InvalidateAnimation(); // an equipment slot changed
		if (InParty())
			Game::Get()->Bar().Refresh();
	}
	return true;
}


void
Actor::ClearInventory()
{
	for (uint32 slot = 0; slot < kNumItemSlots; slot++) {
		IE::item item;
		if (fCRE->GetItemAtSlot(slot, item) && item.name.name[0] != '\0')
			RemoveItem(item.name);
	}
}


void
Actor::GiveAllItemsTo(Actor* target)
{
	for (uint32 slot = 0; slot < kNumItemSlots; slot++) {
		IE::item item;
		if (fCRE->GetItemAtSlot(slot, item) && item.name.name[0] != '\0') {
			if (!target->AddItem(item.name))
				continue;
			RemoveItem(item.name);
		}
	}
}


bool
Actor::EquipItem(const res_ref& itemName)
{
	int32 currentSlot = fCRE->FindItemSlot(itemName);
	if (currentSlot < 0)
		return false;

	ITMResource* itm = gResManager->GetITM(itemName);
	if (itm == NULL)
		return false;
	int32 targetSlot = _DefaultSlotForItemType(itm->ItemType());
	gResManager->ReleaseResource(itm);

	if (targetSlot < 0)
		return false; // this item type is never equipped

	if ((uint32)targetSlot == kSlotWeaponFirst) {
		// A weapon is wielded from whichever quickslot it sits in, or from
		// the first free one it is moved to.
		int32 weaponSlot = currentSlot;
		if (weaponSlot < (int32)kSlotWeaponFirst
				|| weaponSlot >= (int32)(kSlotWeaponFirst + kNumWeaponSlots)) {
			weaponSlot = fCRE->FindFreeSlot(kSlotWeaponFirst,
					kSlotWeaponFirst + kNumWeaponSlots - 1);
			if (weaponSlot < 0)
				return false; // every quickslot is busy - swapping is out of scope
			fCRE->MoveItemBetweenSlots((uint32)currentSlot, (uint32)weaponSlot);
		}
		return SelectWeapon(weaponSlot - kSlotWeaponFirst);
	}

	if ((uint32)targetSlot == (uint32)currentSlot)
		return true; // already in its default slot

	IE::item occupant;
	if (fCRE->GetItemAtSlot((uint32)targetSlot, occupant))
		return false; // target slot busy - swapping is out of scope

	fCRE->MoveItemBetweenSlots((uint32)currentSlot, (uint32)targetSlot);
	// An equipment slot changed hands - same reason TakeItemFromSlot()/
	// MoveItemToSlot() invalidate: the sprite/paperdoll may need
	// rebuilding (armor/weapon layers).
	InvalidateAnimation();
	return true;
}


bool
Actor::UnequipSlot(uint32 slot)
{
	IE::item item;
	if (!fCRE->GetItemAtSlot(slot, item))
		return false;

	int32 freeSlot = fCRE->FindFreeSlot(kSlotGeneralFirst, kSlotGeneralLast);
	if (freeSlot < 0)
		return false;

	fCRE->MoveItemBetweenSlots(slot, (uint32)freeSlot);
	InvalidateAnimation(); // an equipment slot changed
	return true;
}


bool
Actor::MoveItemToSlot(uint32 fromSlot, uint32 toSlot)
{
	if (fromSlot == toSlot)
		return true;
	if (fromSlot >= kNumItemSlots || toSlot >= kNumItemSlots)
		return false;

	int32 fromIndex = fCRE->ItemsIndexAtSlot(fromSlot);
	if (fromIndex < 0)
		return false;

	if (!_SlotAcceptsItemType(toSlot, _ItemTypeAtSlot(fCRE, fromSlot)))
		return false;

	int32 toIndex = fCRE->ItemsIndexAtSlot(toSlot);
	// Swapping: the item currently in toSlot has to be legal back in
	// fromSlot, otherwise the swap would leave it somewhere it can't go.
	if (toIndex >= 0
			&& !_SlotAcceptsItemType(fromSlot, _ItemTypeAtSlot(fCRE, toSlot)))
		return false;

	fCRE->SetItemAtSlot(toSlot, fromIndex);
	fCRE->SetItemAtSlot(fromSlot, toIndex); // -1 clears when toSlot was empty

	// An equipment slot (anything before the general grid) changed hands:
	// the world sprite may need rebuilding (armor/weapon layers).
	if (fromSlot < kSlotGeneralFirst || toSlot < kSlotGeneralFirst) {
		InvalidateAnimation();
		if (InParty())
			Game::Get()->Bar().Refresh();
	}
	return true;
}


bool
Actor::InParty() const
{
	return Game::Get()->Party()->HasActor(this);
}


bool
Actor::IsPersistent() const
{
	return InParty() || Game::Get()->NPCs().Contains(this);
}


res_ref
Actor::AreaName() const
{
	return fAreaName;
}


void
Actor::SetAreaName(const res_ref& name)
{
	fAreaName = name;
}


void
Actor::IncrementNumTimesTalkedTo()
{
	fActor->num_times_talked_to++;
}


void
Actor::SetNumTimesTalkedTo(uint32 num)
{
	fActor->num_times_talked_to = num;
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
Actor::ClickedOn(Object* target, ClickIntent intent)
{
	if (target == NULL)
		return;

	target->Clicked(this);

	// TODO: Add a "mode" to the ClickedOn method, to distinguish
	// an attack from a dialog start, etc

	if (Door* door = dynamic_cast<Door*>(target)) {
		// Each action gets its own action_params (even though both target
		// the same door) because dispatch now reads params->id fresh every
		// tick: sharing one object between two actions queued back-to-back
		// would corrupt the id of whichever one is still executing once the
		// second AddAction() call runs.
		action_params* walkParams = new action_params(Name(), door->Name());
		walkParams->id = 22; // MOVETOOBJECT
		AddAction(walkParams);
		walkParams->Release();

		action_params* openParams = new action_params(Name(), door->Name());
		openParams->id = 143; // OPENDOOR
		AddAction(openParams);
		openParams->Release();
	} else if (Actor* actor = dynamic_cast<Actor*>(target)) {
		if (actor->IsState(STATE_DEAD)) {
			if (intent != CLICK_DEFAULT)
				return; // nobody to talk to, attack or defend
			// Loot the corpse - same two-action MOVETOOBJECT+USECONTAINER
			// queue as the Container branch below (RunActionUseContainer()
			// accepts a dead Actor target too, see its own comment).
			action_params* walkParams = new action_params(Name(), actor->Name());
			walkParams->Second()->globalId = actor->GlobalID();
			walkParams->id = 22; // MOVETOOBJECT
			AddAction(walkParams);
			walkParams->Release();

			action_params* useParams = new action_params(Name(), actor->Name());
			useParams->Second()->globalId = actor->GlobalID();
			useParams->id = 112; // USECONTAINER
			AddAction(useParams);
			useParams->Release();
			return;
		}

		// Same EnemyAlly()-vs-EVILCUTOFF check AreaRoom::_ObjectAtPoint()
		// already uses to pick the hover cursor (CURSOR_TALK/
		// CURSOR_ATTACK) - it just wasn't wired into the actual click
		// yet, so clicking a hostile creature always tried to start a
		// dialog with it instead of attacking.
		if (intent == CLICK_DEFEND) {
			// Same walk-to-the-target queue as a dialog approach, with
			// NIDSPECIAL4 (ProtectObject) as the action.
			action_params* defendParams = new action_params(Name(), actor->Name());
			defendParams->Second()->globalId = actor->GlobalID();
			defendParams->id = 73; // NIDSPECIAL4
			AddAction(defendParams);
			defendParams->Release();
			return;
		}

		const bool talk = intent == CLICK_TALK || (intent == CLICK_DEFAULT
			&& actor->CRE()->EnemyAlly() < IDTable::EnemyAllyValue("EVILCUTOFF"));
		if (talk) {
			// Walk over first, same two-action MOVETOOBJECT+interact queue
			// as the Door/Container branches - RunActionDialog() itself
			// starts the conversation as soon as it runs, with no
			// proximity check of its own (see its own TODO comment), so
			// skipping this walk used to start dialog instantly regardless
			// of distance. That let a real BG1 DLG state's own See(O:Object*)
			// trigger (gating content on the speaker actually seeing the
			// target - e.g. Gorion's Candlekeep ambush warning) evaluate
			// false and silently fall through to unrelated content, since
			// the two actors were never actually brought close together.
			action_params* walkParams = new action_params(Name(), actor->Name());
			walkParams->Second()->globalId = actor->GlobalID();
			walkParams->id = 22; // MOVETOOBJECT
			AddAction(walkParams);
			walkParams->Release();

			action_params* actionParams = new action_params(actor->Name(), Name());
			actionParams->First()->globalId = actor->GlobalID();
			actionParams->id = 8; // DIALOG
			AddAction(actionParams);
			actionParams->Release();
		} else {
			// Self first, target second - same direction the Door
			// branch above uses (RunActionAttack() reads the target via
			// Script::GetTargetObject(), which expects it there), unlike
			// DIALOG's reversed construction above.
			action_params* actionParams = new action_params(Name(), actor->Name());
			actionParams->Second()->globalId = actor->GlobalID();
			actionParams->id = 3; // ATTACK
			AddAction(actionParams);
			actionParams->Release();
		}
	} else if (Container* container = dynamic_cast<Container*>(target)) {
		// Same two-action MOVETOOBJECT+"do the thing" queue as the Door
		// branch above: walk over, then USECONTAINER auto-loots whatever
		// fits into this creature's inventory (no loot GUI - see
		// RunActionUseContainer()).
		action_params* walkParams = new action_params(Name(), container->Name());
		walkParams->id = 22; // MOVETOOBJECT
		AddAction(walkParams);
		walkParams->Release();

		action_params* useParams = new action_params(Name(), container->Name());
		useParams->id = 112; // USECONTAINER
		AddAction(useParams);
		useParams->Release();
	}
}


void
Actor::Shout(int number)
{
	// Like the Infinity Engine (GemRB Map::Shout()), audibility is
	// decided here, at emission time: every actor within earshot gets a
	// "heard" trigger carrying the shouter and the shout number, plus a
	// "LastHeardBy" for the LastHeardBy() object identifier. Heard() is
	// then just a match on that trigger - no second distance check.
	AreaRoom* room = Area();
	if (room == NULL)
		return;
	for (int32 a = 0; a < room->ActorsCount(); a++) {
		Actor* listener = room->ActorAt(a);
		if (listener == NULL || listener == this)
			continue;
		if (room->Distance(listener, this) < kAudibleRange) {
			listener->AddTrigger(trigger_entry("heard", this, number));
			listener->AddTrigger(trigger_entry("LastHeardBy", this));
		}
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


const IE::actor*
Actor::AreaActorEntry() const
{
	return fActor;
}


void
Actor::_HandleScripts()
{
	AddScript(Core::ExtractScript(fActor->script_override), SCRIPT_LEVEL_OVERRIDE);
	// TODO: What is the area script? Wire it up here once resolved:
	// AddScript(Core::ExtractScript(fActor->script_area), SCRIPT_LEVEL_AREA);
	AddScript(Core::ExtractScript(fActor->script_specific), SCRIPT_LEVEL_SPECIFICS);
	AddScript(Core::ExtractScript(fActor->script_class), SCRIPT_LEVEL_CLASS);
	AddScript(Core::ExtractScript(fActor->script_race), SCRIPT_LEVEL_RACE);
	AddScript(Core::ExtractScript(fActor->script_general), SCRIPT_LEVEL_GENERAL);
	AddScript(Core::ExtractScript(fActor->script_default), SCRIPT_LEVEL_DEFAULT);
}


// Selects the ArmorClass field matching a weapon's damage type (IESDP
// itm_v1 offset 0x1c). BG2-specific combo types (6/7/8, e.g. halberds
// dealing crushing+piercing) and unknown values fall back to `effective`
// AC rather than guessing which of the two component types applies.
static int16
_ArmorClassFor(const ArmorClass& ac, uint16 weaponDamageType)
{
	switch (weaponDamageType) {
		case 1: // Piercing/Magic
			return ac.piercing;
		case 2: // Blunt/Crushing
			return ac.crushing;
		case 3: // Slashing
			return ac.slashing;
		case 4: // Missile
			return ac.missile;
		default:
			return ac.effective;
	}
}


void
Actor::AttackTarget(Actor* target)
{
	// Without an explicit round stamp this trigger is pruned by
	// RemoveExpiredTriggers() almost immediately (trigger_entry's round
	// defaults to 0), unlike every other trigger-posting site - see e.g.
	// the "OnCreation"/"Clicked" triggers in Object.cpp.
	trigger_entry triggerEntry("AttackedBy", this);
	triggerEntry.round = Core::Get()->ScriptRound();
	target->AddTrigger(triggerEntry);

	const attack_profile profile = AttackProfile();
	const itm_ability& ability = profile.ability;
	if (profile.spentSlot >= 0)
		ConsumeFromSlot((uint32)profile.spentSlot);

	const ArmorClass targetAC = target->CRE()->AC();
	const int16 effectiveAC = _ArmorClassFor(targetAC, ability.damageType);

	// Melee (fists included) adds the strength bonuses; a missile attack
	// gets none - dexterity's missile bonus isn't modeled.
	const bool melee = !profile.ranged;
	const int32 strengthToHit = melee ? StrengthBonus(CRE(), 0) : 0;
	const int32 strengthDamage = melee ? StrengthBonus(CRE(), 1) : 0;

	// Standard THAC0 to-hit: roll needed = attacker's THAC0 (better with
	// a lower value), minus the weapon's own THAC0 bonus and the strength
	// bonus, minus the target's AC for this damage type (also better/
	// harder to hit when lower). A natural 20 always hits, a natural 1
	// always misses.
	const int32 roll = Core::RollDice(1, 20, 0);
	const int32 neededRoll = CRE()->THAC0() - ability.thac0Bonus
		- strengthToHit - effectiveAC;
	const bool hit = roll == 20 || (roll != 1 && roll >= neededRoll);
	if (!hit)
		return;

	// Unlike AttackedBy above (posted for any attack attempt, hit or
	// miss), HitBy(O:Object*,I:DameType*) only fires on an actual hit -
	// damage-type filtering isn't implemented (see _ArmorClassFor()'s own
	// scope note), so any damage type matches.
	trigger_entry hitBy("HitBy", this);
	hitBy.round = Core::Get()->ScriptRound();
	target->AddTrigger(hitBy);

	// A natural 20 is a critical hit: double damage. (Per-weapon threat
	// ranges - some weapons crit on 19-20 - and criticals-immune targets
	// aren't modeled.)
	int32 damage = Core::RollDice(ability.diceThrown, ability.diceSides,
			ability.damageBonus + strengthDamage);
	if (roll == 20)
		damage *= 2;
	target->ApplyDamage(std::max<int32>(damage, 1));

	for (const spl_effect& effect : profile.onHitEffects)
		target->AddSpellEffect(SpellEffect::FromFeatureBlock(effect, this));
}


int32
Actor::AttackCooldown() const
{
	return fAttackCooldown;
}


void
Actor::SetAttackCooldown(int32 ticks)
{
	fAttackCooldown = ticks;
}


bool
Actor::CanSee(Object* target)
{
	// TODO: Take into account any eventual spell
	if (target == NULL || target == this || !target->IsVisible())
		return false;
	//const IE::point thisPosition = Position();
	//const IE::point targetPosition = target->Position();
	AreaRoom* room = Area();
	if (room != NULL && room->Distance(this, target) < kVisualRange
			&& room->HasLineOfSight(Position(), target->NearestPoint(Position()))) {
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
	_UpdateRegions();
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
Actor::InvalidateAnimation()
{
	fAnimationValid = false;
}


void
Actor::UpdateAnimation(bool ignoreBlocks)
{
	if (!fAnimationValid) {
		delete fCurrentAnimation;
		delete fWeaponAnimation;
		fCurrentAnimation = NULL;
		fWeaponAnimation = NULL;
		if (fAnimationFactory != NULL) {
			fCurrentAnimation = fAnimationFactory->AnimationFor(this, fColors);
			fWeaponAnimation = fAnimationFactory->WeaponOverlayFor(this);
		}
		fAnimationValid = true;
	} else if (fCurrentAnimation != NULL) {
		if (fCurrentAnimation->IsLastFrame()) {
			if (fAnimationAutoSwitchOnEnd) {
				fAnimationAutoSwitchOnEnd = false;
				fAnimationAction = fNextAnimationAction;
				delete fCurrentAnimation;
				delete fWeaponAnimation;
				fCurrentAnimation = fAnimationFactory->AnimationFor(this, fColors);
				fWeaponAnimation = fAnimationFactory->WeaponOverlayFor(this);
			}
			if (fAnimationAction != ACT_DEAD) {
				fCurrentAnimation->NextFrame();
				if (fWeaponAnimation != NULL)
					fWeaponAnimation->NextFrame();
			}
		} else {
			fCurrentAnimation->NextFrame();
			if (fWeaponAnimation != NULL)
				fWeaponAnimation->NextFrame();
		}
	}
}


bool
Actor::MoveToNextPointInPath(bool ignoreBlocks)
{
	if (fPath == NULL)
		return false;

	if (!fPath->IsEmpty() && !fPath->IsEnd()) {
		IE::point nextPoint = fPath->NextStep(fSpeed);
		SetOrientation(nextPoint);
		_SetPositionPrivate(nextPoint);
		SetAnimationAction(ACT_WALKING);

		return true;
	}

	SetAnimationAction(ACT_STANDING);

	delete fPath;
	fPath = NULL;
	return false;
}


void
Actor::_SetOrientation(const IE::point& nextPoint)
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
Actor::_SetPositionPrivate(const IE::point& point, bool triggerTravel)
{
	AreaRoom* room = Area();
	if (room != NULL) {
		room->SearchMap()->ClearPoint(fActor->position.x, fActor->position.y);
	}

	fActor->position = point;

	if (room != NULL) {
		room->SearchMap()->SetPoint(fActor->position.x, fActor->position.y);
	}

	_UpdateRegions(triggerTravel);
}


void
Actor::RefreshColors()
{
	_HandleColors();
	InvalidateAnimation();
}


// Populates fColors from this CRE's own literal color bytes, which
// AnimationFactory::AnimationFor() then hands to Animation for
// _ApplyColorMODs() to recolor the BAM's hair/skin/armor/etc. palette
// ranges with (see Animation.cpp - skipped entirely when fColors is
// NULL). The real engine only ever substitutes a color byte >=200 
// (RANDCOLR.2DA's own placeholder convention for "pick one of N alternatives"
// see randcolr.2da's docs); any other byte is its own literal color index
// regardless of whether the table exists. RANDCOLR.2DA is BG2/ToB/BGEE-
// only content that BG1's CRE data never needed (no BG1 creature's
// color bytes use the >=200 convention), not a prerequisite for
// coloring to work at all.
void
Actor::_HandleColors()
{
	delete fColors;
	fColors = NULL;

	TWODAResource* randColors = gResManager->ResourceExists("RANDCOLR", RES_2DA)
			? gResManager->Get2DA("RANDCOLR") : NULL;

	CREColors originalColors = CRE()->Colors();
	fColors = new CREColors();
	fColors->hair = _GetRandomColor(randColors, originalColors.hair);
	fColors->leather = _GetRandomColor(randColors, originalColors.leather);
	fColors->armor = _GetRandomColor(randColors, originalColors.armor);
	fColors->metal = _GetRandomColor(randColors, originalColors.metal);
	fColors->major = _GetRandomColor(randColors, originalColors.major);
	fColors->minor = _GetRandomColor(randColors, originalColors.minor);
	fColors->skin = _GetRandomColor(randColors, originalColors.skin);

	if (randColors != NULL)
		gResManager->ReleaseResource(randColors);
}


// `randColors` is NULL when RANDCOLR.2DA doesn't exist (BG1) - `index`
// is then always already a real, literal color index (see this method's
// caller's own comment), so it's returned unchanged, same as when the
// table exists but `index` just isn't one of its placeholder codes.
uint8
Actor::_GetRandomColor(TWODAResource* randColors, uint8 index) const
{
	if (randColors == NULL)
		return index;

	uint8 num = index;
	// get column requested index
	for (int32 column = 0; column < randColors->CountColumns(); column++) {
		uint32 value = randColors->IntegerValueAt(0, column);
		if (value == index) {
			// First column is the column name, so we start from 1
			uint32 rndNumber = Core::RandomNumber(1, randColors->CountRows() - 1);
			num = randColors->IntegerValueAt(rndNumber, column);
			break;
		}
	}

	return num;
}


bool
Actor::EvaluateDialogTriggers(std::vector<trigger_params*>& triggers)
{
	if (triggers.empty())
		return false;

	return Script::EvaluateTriggerList(this, triggers);
}


void
Actor::_UpdateRegions(bool triggerTravel)
{
	// An actor parked outside any loaded area (a global NPC) has no regions.
	if (Area() == NULL)
		return;

	BackMap* backMap = Area()->BackMap();
	if (backMap == NULL)
		return;

	if (fRegion != nullptr) {
		if (!fRegion->Contains(Position())) {
			fRegion->ActorExited(this);
			fRegion = nullptr;
		} else
			return;
	}

	::TileCell* tileCell = backMap->TileAtPoint(Position());
	std::vector<Region*> regions;
	if (tileCell != NULL) {
		tileCell->GetRegions(regions);
		for (auto region: regions) {
			if (region->Contains(Position())) {
				fRegion = region;
				region->ActorEntered(this);
				// Leaving the area through a travel region: only a party
				// member's own arrival triggers it (walking a stray
				// monster/NPC through one shouldn't send the player
				// somewhere), and only by actually walking in - not by
				// merely clicking the region, which used to change area
				// on the spot regardless of anything blocking the way
				// there (a closed door, ...). RequestAreaChange(), not
				// LoadArea() directly: this runs from inside
				// AreaRoom::Update()'s own actor-update loop (see its own
				// comment on the heap-use-after-free that caused).
				//
				// `this` is also passed as the actor whose action list
				// RequestAreaChange() should clear once it's safe to do so
				// (see its own comment) - the action that walked this
				// actor here is done, and _UnloadArea() deliberately
				// leaves a party member's action list alone otherwise
				// (needed elsewhere for a scripted cutscene transition to
				// keep an in-progress action, e.g. FADEFROMCOLOR, running
				// in the new area) - without clearing it, a stale
				// MOVETOPOINT still holding this area's own coordinates
				// would resume right after AreaRoom's constructor
				// repositions every party member to the new entrance,
				// walking this actor straight off to a bogus spot in the
				// new area's map. Clearing it here instead, synchronously,
				// is NOT safe in general: _UpdateRegions() can also run
				// synchronously from deep inside this very actor's own
				// action execution (e.g. JUMPTOPOINT's handler calling
				// SetPosition() directly, itself still reading its own
				// action_params after this returns) - a real, reproduced
				// heap-use-after-free (the action_params freed here, out
				// from under its own still-running handler).
				if (triggerTravel && region->Type() == IE::REGION_TYPE_TRAVEL && InParty()) {
					Core::Get()->RequestAreaChange(region->DestinationArea(),
						"foo", region->DestinationEntrance(), this);
				}
			}
		}
	}

	// Wilderness map edge (see SearchMap::IsWorldmapExit()'s own comment)
	// - entirely separate from the Region-based check above: there's no
	// Region object here at all, just a search-map cell classification.
	// Same InParty()-only gating as the travel region case (a stray
	// monster wandering to the map edge shouldn't open the worldmap).
	// RequestWorldMapLoad() defers exactly like RequestAreaChange() does,
	// for the same reason (this runs from inside AreaRoom::Update()'s own
	// actor loop).
	//
	// fOnWorldmapExit debounces this the same way fRegion above debounces
	// travel regions: without it, this fires on *every* tick the party
	// happens to be standing on such a cell, not just the tick it walks
	// onto one - harmless while actually walking through (the area
	// unloads before another tick runs), but a real, reproduced bug the
	// moment the party is placed back on that same cell without walking
	// there, e.g. WorldMap::MouseDown()'s own Core::ReturnFromWorldMap()
	// call when the party left through an edge and then immediately
	// clicks back to where they already are: the worldmap would reopen
	// on the very next tick, looking like the click did nothing.
	if (InParty()) {
		SearchMap* searchMap = Area()->SearchMap();
		bool onExit = searchMap != NULL
			&& searchMap->IsWorldmapExit(Position().x, Position().y);
		if (onExit && !fOnWorldmapExit) {
			int32 direction = searchMap->EdgeDirection(Position().x, Position().y);
			Core::Get()->RequestWorldMapLoad(direction);
		}
		fOnWorldmapExit = onExit;
	}
}
