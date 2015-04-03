#ifndef __ACTOR_H
#define __ACTOR_H

#include "Bitmap.h"
#include "GraphicsDefs.h"
#include "IETypes.h"
#include "Object.h"

#include <list>
#include <vector>

const static uint32 kNumAnimations = 8;
const static uint32 kNumActions = 2;

class Animation;
class AnimationFactory;
class Bitmap;
class BCSResource;
class CREResource;
class PathFinder;
class Script;

class Actor : public Object {
public:
	Actor(IE::actor& actor);
	Actor(IE::actor& actor, CREResource* cre);
	Actor(const char* creName, IE::point position, int face);
	virtual ~Actor();

	CREResource *CRE() const;

	bool IsNew() const;

	const ::Bitmap* Bitmap() const;
	GFX::rect Frame() const;

	int Orientation() const;
	void SetOrientation(int o);

	IE::point Position() const;
	void SetPosition(const IE::point& position);

	IE::point Destination() const;
	void SetDestination(const IE::point &dest);

	bool Spawned() const;

	uint32 NumTimesTalkedTo() const;

	void InitiateDialogWith(Actor* actor);

	void SetArea(const char* name);
	const char* Area() const;

	virtual IE::point NearestPoint(const IE::point& point) const;
	virtual void ClickedOn(Object* target);

	void Shout(int number);

	void SetFlying(bool fly);
	bool IsFlying() const;

	void Select(bool select);
	bool IsSelected() const;

	void SetInterruptable(const bool interrupt);
	bool IsInterruptable() const;

	Script* MergeScripts();

	bool SkipConditions() const;
	void StopCheckingConditions();

	void UpdateSee();
	void SetSeen(Object* object);
	bool HasSeen(const Object* object) const;

	bool IsReachable(const IE::point& pt) const;

	// TODO: Merge and clean this mess
	void SetAnimationAction(int action);
	void UpdateAnimation(bool ignoreBlocks);
	void MoveToNextPointInPath(bool ignoreBlocks);

private:
	IE::actor *fActor;
	AnimationFactory* fAnimationFactory;
	Animation* fCurrentAnimation;

	std::string fArea;

	CREResource *fCRE;
	bool fOwnsActor;

	bool fDontCheckConditions;
	bool fIsInterruptable;

	bool fFlying;
	bool fSelected;

	bool fAttacking;
	int fAction;

	PathFinder* fPath;
	int fSpeed;

	void _Init();
	void _AddScript(Script*& destination, const res_ref& scriptName);
	void _SetOrientation(const IE::point& nextPoint);
	void _SetOrientationExtended(const IE::point& nextPoint);


};

#endif //__ACTOR_H
