#ifndef __ACTOR_H
#define __ACTOR_H

#include "Bitmap.h"
#include "IETypes.h"
#include "Object.h"

#include <string>
#include <vector>

const static uint32 kNumAnimations = 8;
const static uint32 kNumActions = 2;

struct CREColors;

// Weapon handedness/ranged category, used by the "Character" avatar-naming
// scheme (docs/iesdp-gh-pages/appendices/avatarnaming.htm): Action='A'
// detail digit 1 (1H) / 2 (2H) for a melee weapon, or Action='S' with an
// empty ("bow") / "x" ("crossbow") suffix for a ranged one. Unarmed (no
// weapon equipped) is one-handed melee - every field stays false.
struct WeaponAnimationType {
	bool isTwoHanded = false;
	bool isRanged = false;
	bool isCrossbow = false;
};

class Animation;
class AnimationFactory;
class AreaRoom;
class Bitmap;
class BCSResource;
class CREResource;
class ITMResource;
class Path;
class Region;
class Script;
class TileCell;
class TWODAResource;
class Actor : public Object {
public:
	Actor(IE::actor& actor);
	Actor(IE::actor& actor, CREResource* cre);
	Actor(const char* creName, IE::point position, int face);

	std::string LongName() const;
	// Overwrites the display name (fActor->name) - used by character
	// creation, where the scripting/lookup name stays "PLAYER1" but the
	// name shown above the sprite and in the UI is the player's choice.
	void SetLongName(const char* name);

	virtual void Print() const;

	CREResource *CRE() const;

	// The ARE actor struct this Actor aliases (fActor) - exposed
	// read-only so ARAResource can recognize which of its own table
	// entries a given Actor came from (ARAResource::IndexOfActorEntry())
	// without handing out write access to the struct itself. Not
	// necessarily one of that area's own entries - an actor spawned at
	// runtime (CreateCreature* etc.) owns a standalone struct instead
	// (fOwnsActor), never part of any ARE's own actor table to begin
	// with; IndexOfActorEntry() bounds-checks and returns -1 for those.
	const IE::actor* AreaActorEntry() const;

	const ::Bitmap* Bitmap() const;
	IE::rect Frame() const;

	int Orientation() const;
	void SetOrientation(int o);
	void SetOrientation(const IE::point& toPoint);

	IE::point Position() const;
	void SetPosition(const IE::point& position);

	IE::point Destination() const;
	void SetDestination(const IE::point &dest, bool ignoreSearchMap = false);
	void ClearDestination();

	// SETHOMELOCATION(P:Point*) - not consumed by anything yet (no
	// "return home" AI behavior exists in this engine), stored for
	// whenever that's added.
	IE::point HomeLocation() const;
	void SetHomeLocation(const IE::point& position);

	void Draw(AreaRoom* room) const;

	IE::point RestrictionDistance() const;
	virtual IE::point NearestPoint(const IE::point& start) const;

	bool IsWalking() const;

	void SetRegion(Region* region);
	Region* CurrentRegion() const;

	bool Spawned() const;

	bool InParty() const;

	std::string ArmorAnimation() const;
	std::string WeaponAnimation() const;
	WeaponAnimationType EquippedWeaponAnimationType() const;

	// The paperdoll (PLT) resref for this actor's inventory screen - see
	// AnimationFactory::PaperdollName(). Empty if this actor has no
	// animation factory (shouldn't normally happen for a fully
	// initialized Actor).
	std::string PaperdollName() const;

	// Resolves the item in the "weapon 1" quickslot (slot 9 - same slot
	// ArmorAnimation()/WeaponAnimation() above already look up for their
	// own purposes). Returns NULL if the slot is empty (caller should
	// fall back to unarmed/fists). Like any other resource fetched via
	// ResourceManager, the caller must gResManager->ReleaseResource() it.
	ITMResource* EquippedWeapon() const;

	// Adds a new item (by resref) to this actor's inventory, in the first
	// free general-inventory slot (15-34, per SLOTS.IDS). Returns false
	// (logging why) if the item resource doesn't exist, or if the CRE's
	// on-disk Items table/inventory slots have no free entry - this
	// engine doesn't grow a CRE's item table, so this only works while
	// the creature has spare capacity (most placed creatures do).
	bool AddItem(const res_ref& itemName, uint16 quantity = 1);

	// Removes one item (by resref, wherever it currently sits - equipped
	// or not) from this actor's inventory. Returns false if not found.
	bool RemoveItem(const res_ref& itemName);

	// Removes whatever occupies the given CRE item slot, handing back its
	// full IE::item (name + quantities) in `out`. Returns false for an
	// empty/invalid slot. Used to drop an item onto the area floor.
	bool TakeItemFromSlot(uint32 slot, IE::item& out);

	// Removes every item in every inventory slot (equipped or not) - no
	// "item lying on the ground" object type exists in this engine, so
	// (unlike the real DropInventory()) nothing is placed anywhere; this
	// is really a destroy-everything operation. Used by both DROPINVENTORY
	// and DESTROYALLEQUIPMENT, which are equivalent under this
	// simplification.
	void ClearInventory();

	// Moves every item in every inventory slot (equipped or not) to
	// target's inventory. An item target has no room for (Items table or
	// general slots full) is skipped and stays with this actor instead of
	// stopping the whole transfer.
	void GiveAllItemsTo(Actor* target);

	// Moves an already-owned item (by resref) into its default slot for
	// its ITM type (weapon -> "weapon 1", armor -> armor slot, etc - see
	// the .cpp for the full mapping). Returns false if the actor doesn't
	// have the item, or its default slot is already occupied.
	bool EquipItem(const res_ref& itemName);

	// Moves whatever is in `slot` back to a free general-inventory slot.
	// Returns false if the slot is empty, or there's no free slot to
	// move it to.
	bool UnequipSlot(uint32 slot);

	// Moves the item in `fromSlot` into `toSlot` (both are CRE item-array
	// indices, see CreResource.h's kSlot* constants). If `toSlot` already
	// holds an item the two are swapped. Fails (returns false, leaving
	// both slots untouched) if `fromSlot` is empty, or if either item's
	// ITM type isn't allowed in the slot it would end up in (e.g. armor
	// into a weapon slot). Recomputes appearance when an equipment slot
	// is involved. This is the low-level move behind the inventory GUI's
	// drag/drop; unlike EquipItem() it takes an explicit destination and
	// can swap.
	bool MoveItemToSlot(uint32 fromSlot, uint32 toSlot);

	bool IsEqual(const Actor* object) const;

	bool IsEnemyOf(const Actor* object) const;
	bool IsName(const char* name) const;
	bool IsClass(int c) const;
	bool IsRace(int race) const;
	bool IsGender(int gender) const;
	bool IsGeneral(int general) const;
	bool IsSpecific(int specific) const;
	bool IsAlignment(int alignment) const;
	bool IsEnemyAlly(int ea) const;
	void SetEnemyAlly(int ea);
	bool IsState(int state) const;

	// Subtracts amount from the current hit points (clamped at 0). If this
	// brings HP to 0 and the actor isn't already dead, also transitions it
	// to STATE_DEAD (animation + death variable) - see the .cpp for
	// details. Used by both melee (AttackTarget()) and spell damage
	// (SpellEffect.cpp's opcode #12 handler), so death handling only
	// lives in one place.
	void ApplyDamage(int32 amount);

	// Adds `amount` to this creature's total XP (CRE()->Experience()) and,
	// if the new total crosses one or more XPLEVEL.2DA thresholds for its
	// class(es), applies the resulting level-up(s) - see the .cpp's
	// _CheckLevelUp() for what a level-up actually changes.
	void GainExperience(uint32 amount);

	bool MatchNode(object_params* node) const;

	Actor* ResolveIdentifier(const int identifier) const;

	// Resolves one to-hit + damage attack against target right now (see
	// the .cpp for the formula). Callers are responsible for their own
	// round pacing - see AttackCooldown()/SetAttackCooldown() below,
	// used by RunActionAttack() (scripting/Actions.cpp) so this doesn't
	// fire every single engine tick.
	void AttackTarget(Actor* object);

	// Ticks remaining before this actor's next attack is allowed to
	// resolve; sender-side round pacing lives in the caller (see
	// RunActionAttack()), not in AttackTarget() itself.
	int32 AttackCooldown() const;
	void SetAttackCooldown(int32 ticks);

	void IncrementNumTimesTalkedTo();
	void SetNumTimesTalkedTo(uint32 num);
	uint32 NumTimesTalkedTo() const;

	virtual void ClickedOn(Object* target);

	void Shout(int number);

	void SetFlying(bool fly);
	bool IsFlying() const;

	void Select(bool select);
	bool IsSelected() const;

	bool CanSee(Object* target);

	bool IsReachable(const IE::point& pt) const;

	virtual void Update(bool scripts);

	int AnimationAction() const;
	void SetAnimationAction(int action);

	void UpdateAnimation(bool ignoreBlocks);
	// Forces the world sprite to be rebuilt on the next UpdateAnimation()
	// - call after something that changes the actor's look but not its
	// orientation/action (e.g. equipping armor or a weapon).
	void InvalidateAnimation();
	bool MoveToNextPointInPath(bool ignoreBlocks);

	void SetText(const std::string& string);
	std::string Text() const;

	bool EvaluateDialogTriggers(std::vector<trigger_params*>& triggers);

	static bool PointPassableTrue(const IE::point& point) { return true; };

private:
	virtual ~Actor();

	void _ClearItemSlot(uint32 slot);

	IE::actor *fActor;
	AnimationFactory* fAnimationFactory;
	Animation* fCurrentAnimation;
	int fAnimationAction;
	int fNextAnimationAction;
	bool fAnimationValid;
	bool fAnimationAutoSwitchOnEnd;

	CREResource *fCRE;
	bool fOwnsActor;

	CREColors* fColors;

	bool fFlying;
	bool fSelected;

	bool fAttacking;
	int32 fAttackCooldown;

	Path* fPath;
	int fSpeed;

	Region* fRegion;

	std::string fText;

	int fSelectedRadius;
	int fSelectedRadiusStep;

	IE::point fHomeLocation = { 0, 0 };

	void _Init();
	void _HandleScripts();
	void _SetPositionPrivate(const IE::point& point);

	::Script* _ExtractScript(const res_ref& scriptName);
	void _SetOrientation(const IE::point& nextPoint);
	void _SetOrientationExtended(const IE::point& nextPoint);
	void _DrawActorText(AreaRoom* room) const;
	void _DrawActorName(AreaRoom* room) const;
	void _DrawActorPath(AreaRoom* room) const;
	void _DrawCircle(AreaRoom* room) const;
	void _HandleColors();
	uint8 _GetRandomColor(TWODAResource* resource, uint8 index) const;

	void _UpdateRegions();

	void _CheckLevelUp();
};

struct ZOrderSorter {
	bool operator()(Actor* const& actor1, Actor* const& actor2) const {
		return actor1->Position().y < actor2->Position().y;
	}
};

#endif //__ACTOR_H
