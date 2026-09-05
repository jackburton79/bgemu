#include "Actor.h"

#include "2DAResource.h"
#include "Animation.h"
#include "AnimationFactory.h"
#include "AreaRoom.h"
#include "BackMap.h"
#include "BamResource.h"
#include "Bitmap.h"
#include "Core.h"
#include "CreResource.h"
#include "Door.h"
#include "Game.h"
#include "GraphicsEngine.h"
#include "ITMResource.h"
#include "Log.h"
#include "Party.h"
#include "PathFind.h"
#include "Region.h"
#include "ResManager.h"
#include "SearchMap.h"
#include "Script.h"
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
	fAttackCooldown(0),
	fPath(NULL),
	fSpeed(2),
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
	fAttackCooldown(0),
	fPath(NULL),
	fSpeed(2),
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
	fAttackCooldown(0),
	fPath(NULL),
	fSpeed(2),
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
#if 1
		_HandleColors();
#endif
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
			Core::Get()->Game() == game::GAME_BALDURSGATE) ||
			fActor->orientation > IE::ORIENTATION_EXT_SSE) {
		std::cerr << "Weird orientation " << fActor->orientation << std::endl;
		fActor->orientation = 0;
	}

	SetActive(true);

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
		return nullptr;
		//throw std::runtime_error(message);
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
	if (Core::Get()->Game() == game::GAME_BALDURSGATE)
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
	if (fPath == NULL)
		fPath = new Path(); //(PathFinder::kStep, AreaRoom::IsPointPassable);

	IE::point destination = fActor->position;

	test_function func;
	if (ignoreSearchMap)
		func = Actor::PointPassableTrue;
	else
		func = AreaRoom::IsPointPassable;
	try {
		fPath->Set(fActor->position, point, func);
		destination = fPath->End();
	} catch (...) {
		std::cerr << Log::Red << Name() << ": Actor::SetDestination() failed!" << Log::Normal << std::endl;
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
				if (sides > 0 && rolls > 0)
					hpGain += Core::RollDice(rolls, sides, 0);
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
static const uint32 kSlotHelmet = 0;
static const uint32 kSlotArmor = 1;
static const uint32 kSlotShield = 2;
static const uint32 kSlotGauntlets = 3;
static const uint32 kSlotRingLeft = 4;
static const uint32 kSlotAmulet = 6;
static const uint32 kSlotBelt = 7;
static const uint32 kSlotBoots = 8;
static const uint32 kSlotWeaponFirst = 9;
static const uint32 kSlotAmmoFirst = 13;
static const uint32 kSlotAmmoLast = 16;
static const uint32 kSlotCloak = 17;
static const uint32 kSlotGeneralFirst = 21;
static const uint32 kSlotGeneralLast = 36;


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
Actor::WeaponAnimation() const
{
	// TODO: Refactor: items should be loaded elsewhere
	IE::item weapon;
	if (fCRE->GetItemAtSlot(kSlotWeaponFirst, weapon)) {
		ITMResource* itm = gResManager->GetITM(weapon.name);
		if (itm != NULL) {
			std::string animationString = itm->Animation();
			gResManager->ReleaseResource(itm);
			return animationString;
		}
	}

	return "";
}


ITMResource*
Actor::EquippedWeapon() const
{
	// TODO: Refactor: items should be loaded elsewhere (same slot lookup
	// as WeaponAnimation() above)
	IE::item weapon;
	if (!fCRE->GetItemAtSlot(kSlotWeaponFirst, weapon))
		return NULL;

	return gResManager->GetITM(weapon.name);
}


bool
Actor::AddItem(const res_ref& itemName, uint16 quantity)
{
	ITMResource* itm = gResManager->GetITM(itemName);
	if (itm == NULL) {
		std::cerr << Name() << ": AddItem(" << itemName.CString()
				<< "): no such item resource" << std::endl;
		return false;
	}
	gResManager->ReleaseResource(itm);

	int32 itemsIndex = fCRE->FindFreeItemsEntry();
	if (itemsIndex < 0) {
		std::cerr << Name() << ": AddItem(" << itemName.CString()
				<< "): no free Items table entry" << std::endl;
		return false;
	}

	int32 slot = fCRE->FindFreeSlot(kSlotGeneralFirst, kSlotGeneralLast);
	if (slot < 0) {
		std::cerr << Name() << ": AddItem(" << itemName.CString()
				<< "): no free inventory slot" << std::endl;
		return false;
	}

	IE::item item;
	item.name = itemName;
	item.expiration_time = 0;
	item.expiration_time2 = 0;
	item.quantity1 = quantity;
	item.quantity2 = 0;
	item.quantity3 = 0;
	item.flags = 1; // Identified

	fCRE->SetItemAtItemsIndex((uint16)itemsIndex, item);
	fCRE->SetItemAtSlot((uint32)slot, itemsIndex);
	return true;
}


bool
Actor::RemoveItem(const res_ref& itemName)
{
	int32 slot = fCRE->FindItemSlot(itemName);
	if (slot < 0)
		return false;

	// Zero the Items-table entry so FindFreeItemsEntry() can reuse it for
	// a future AddItem(), then unlink the slot.
	int32 itemsIndex = fCRE->ItemsIndexAtSlot((uint32)slot);
	IE::item empty;
	empty.name = res_ref();
	empty.expiration_time = 0;
	empty.expiration_time2 = 0;
	empty.quantity1 = 0;
	empty.quantity2 = 0;
	empty.quantity3 = 0;
	empty.flags = 0;
	fCRE->SetItemAtItemsIndex((uint16)itemsIndex, empty);
	fCRE->SetItemAtSlot((uint32)slot, -1);

	return true;
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
	int32 targetSlot = _DefaultSlotForItemType(itm->Type());
	gResManager->ReleaseResource(itm);

	if (targetSlot < 0)
		return false; // this item type is never equipped

	if ((uint32)targetSlot == (uint32)currentSlot)
		return true; // already in its default slot

	IE::item occupant;
	if (fCRE->GetItemAtSlot((uint32)targetSlot, occupant))
		return false; // target slot busy - swapping is out of scope

	fCRE->MoveItemBetweenSlots((uint32)currentSlot, (uint32)targetSlot);
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
	return true;
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
		if (actor->IsState(STATE_DEAD))
			return; // can't start a conversation with a corpse

		action_params* actionParams = new action_params(actor->Name(), Name());
		actionParams->id = 8; // DIALOG
		AddAction(actionParams);
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

	itm_ability ability;
	bool hasAbility = false;
	ITMResource* weapon = EquippedWeapon();
	if (weapon != NULL) {
		hasAbility = weapon->GetAbility(0, ability);
		gResManager->ReleaseResource(weapon);
	}
	if (!hasAbility) {
		// Unarmed, or the equipped item has no usable ability: there's no
		// dedicated "fists" ITM resource to load, so fall back to a small
		// hardcoded unarmed profile instead.
		ability.attackType = 1; // Melee
		ability.thac0Bonus = 0;
		ability.diceSides = 2;
		ability.diceThrown = 1;
		ability.damageBonus = 0;
		ability.damageType = 5; // Fists
	}

	const ArmorClass targetAC = target->CRE()->AC();
	const int16 effectiveAC = _ArmorClassFor(targetAC, ability.damageType);

	// Standard THAC0 to-hit: roll needed = attacker's THAC0 (better with
	// a lower value), minus the weapon's own THAC0 bonus, minus the
	// target's AC for this damage type (also better/harder to hit when
	// lower). A natural 20 always hits, a natural 1 always misses.
	const int32 roll = Core::RollDice(1, 20, 0);
	const int32 neededRoll = CRE()->THAC0() - ability.thac0Bonus - effectiveAC;
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

	const int32 damage = Core::RollDice(ability.diceThrown, ability.diceSides,
			ability.damageBonus);
	target->ApplyDamage(damage);
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
	}

	fActor->position = point;

	if (room != NULL) {
		room->SearchMap()->SetPoint(fActor->position.x, fActor->position.y);
	}

	_UpdateRegions();
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

	for (auto*& triggerNode : triggers) {
		int orTrig = 0;
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


void
Actor::_UpdateRegions()
{
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
			}
		}
	}
}
