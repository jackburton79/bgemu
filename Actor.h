#ifndef __ACTOR_H
#define __ACTOR_H

#include "Bitmap.h"
#include "GraphicsDefs.h"
#include "IETypes.h"
#include "Object.h"

#include <list>
#include <string>
#include <vector>

const static uint32 kNumAnimations = 8;
const static uint32 kNumActions = 2;

struct CREColors;
class Animation;
class AnimationFactory;
class AreaRoom;
class Bitmap;
class BCSResource;
class CREResource;
class PathFinder;
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

	virtual void Print() const;

	CREResource *CRE() const;

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

	bool MatchNode(object_params* node) const;

	Actor* ResolveIdentifier(const int identifier) const;

	void AttackTarget(Actor* object);

	void IncrementNumTimesTalkedTo();
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
	void MoveToNextPointInPath(bool ignoreBlocks);

	void UpdateTileCell();
	void SetTileCell(::TileCell*);

	void SetText(const std::string& string);
	std::string Text() const;

	bool EvaluateDialogTriggers(std::vector<trigger_params*>& triggers);

	static bool PointPassableTrue(const IE::point& point) { return true; };

private:
	virtual ~Actor();

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

	PathFinder* fPath;
	int fSpeed;

	::TileCell* fTileCell;
	Region* fRegion;

	std::string fText;

	int fSelectedRadius;
	int fSelectedRadiusStep;

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
};

struct ZOrderSorter {
	bool operator()(Actor* const& actor1, Actor* const& actor2) const {
		return actor1->Position().y < actor2->Position().y;
	}
};

#endif //__ACTOR_H
