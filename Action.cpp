#include "Action.h"
#include "Actor.h"
#include "Animation.h"
#include "Core.h"
#include "Door.h"
#include "Timer.h"

#include <algorithm>

static bool
PointSufficientlyClose(const IE::point& pointA, const IE::point& pointB)
{
	return (std::abs(pointA.x - pointB.x) <= 5 * 2)
		&& (std::abs(pointA.y - pointB.y) <= 5 * 2);
}


Action::Action(Object* object)
    :
	fObject(object),
	fInitiated(false),
	fCompleted(false)
{
}


Action::~Action()
{
}


bool
Action::Initiated() const
{
	return fInitiated;
}


bool
Action::Completed() const
{
    return fCompleted;
}


/* virtual */
void
Action::operator()()
{
	if (!fInitiated)
		fInitiated = true;
}


// ActionWithTarget
ActionWithTarget::ActionWithTarget(Object* object, Object* target)
	:
	Action(object),
	fTarget(target)
{
}


void
ActionWithTarget::operator()()
{
	Action::operator()();
}


// WalkTo
WalkTo::WalkTo(Object* object, IE::point destination)
	:
	Action(object),
	fDestination(destination)
{
}


/* virtual */
void
WalkTo::operator()()
{
	Actor* actor = dynamic_cast<Actor*>(fObject);
	if (actor == NULL)
		return;

	if (!Initiated())
		actor->SetDestination(fDestination);

	Action::operator()();

	if (actor->Position() == actor->Destination()) {
		fCompleted = true;
		actor->SetAnimationAction(ACT_STANDING);
		return;
	}

	actor->SetAnimationAction(ACT_WALKING);
	actor->MoveToNextPointInPath(actor->IsFlying());
}


// FlyTo
FlyTo::FlyTo(Object* object, IE::point destination, int time)
	:
	Action(object),
	fDestination(destination)
{
}


/* virtual */
void
FlyTo::operator()()
{
	Actor* actor = dynamic_cast<Actor*>(fObject);
	if (actor == NULL)
		return;

	if (!Initiated())
		actor->SetDestination(fDestination);

	Action::operator()();

	if (actor->Position() == actor->Destination()) {
		fCompleted = true;
		actor->SetAnimationAction(ACT_STANDING);
		return;
	}

	actor->SetAnimationAction(ACT_WALKING);
	actor->MoveToNextPointInPath(true);
}


// Toggle
Toggle::Toggle(Object* object, Door* door)
	:
	Action(object),
	fDoor(door)
{
}


/* virtual */
void
Toggle::operator()()
{
	Action::operator()();
	fDoor.Target()->Toggle();
	fCompleted = true;
}


// Attack
Attack::Attack(Object* object, Object* target)
	:
	ActionWithTarget(object, target)
{
}


/* virtual */
void
Attack::operator()()
{
	Actor* actor = dynamic_cast<Actor*>(fObject);
	if (actor == NULL)
		return;
	Actor* target = dynamic_cast<Actor*>(fTarget);
	if (target == NULL)
		return;

	if (!Initiated()) {
		IE::point point = fTarget->NearestPoint(actor->Position());
		if (!PointSufficientlyClose(actor->Position(), point))
			actor->SetDestination(point);
		Action::operator()();
	}

	if (actor->Position() != actor->Destination()) {
		actor->SetAnimationAction(ACT_WALKING);
		actor->MoveToNextPointInPath(actor->IsFlying());
	} else {
		actor->SetAnimationAction(ACT_ATTACKING);
		actor->AttackTarget(target);
		fCompleted = true;
	}
}


// RunAwayFrom
RunAwayFrom::RunAwayFrom(Object* object, Object* target)
	:
	ActionWithTarget(object, target)
{
}


/* virtual */
void
RunAwayFrom::operator()()
{
	Actor* actor = dynamic_cast<Actor*>(fObject);
	if (actor == NULL)
		return;

	Action::operator()();

	// TODO: Improve.
	// TODO: We are recalculating this every time. Is it correct ?
	if (Core::Get()->Distance(actor, fTarget) < 200) {
		IE::point point = PointAway();
		if (actor->Destination() != point) {
			actor->SetDestination(point);
		}
	}

	if (actor->Position() == actor->Destination()) {
		fCompleted = true;
		actor->SetAnimationAction(ACT_STANDING);
	} else {
		actor->SetAnimationAction(ACT_WALKING);
		actor->MoveToNextPointInPath(actor->IsFlying());
	}
}


IE::point
RunAwayFrom::PointAway() const
{
	IE::point point = fObject->Position();

	if (fTarget->Position().x > fObject->Position().x)
		point.x -= 100;
	else if (fTarget->Position().x < fObject->Position().x)
		point.x += 100;

	if (fTarget->Position().y > fObject->Position().y)
		point.y -= 100;
	else if (fTarget->Position().y < fObject->Position().y)
		point.y += 100;

	return point;
}


// Dialogue
Dialogue::Dialogue(Object* source, Object* target)
	:
	ActionWithTarget(source, target)
{

}


/* virtual */
void
Dialogue::operator()()
{
	Actor* actor = dynamic_cast<Actor*>(fObject);
	if (actor == NULL)
		return;
	
	Actor* target = dynamic_cast<Actor*>(fTarget);
	if (target == NULL)
		return;

	// TODO: Some dialogue action require the actor to be near the target,
	// others do not. Must be able to differentiate
/*
	const IE::point point = fTarget->Target()->NearestPoint(fActor.Target()->Position());
	if (!PointSufficientlyClose(fActor.Target()->Destination(), point))
		fActor.Target()->SetDestination(point);
*/
	Action::operator()();
/*
	if (!PointSufficientlyClose(fActor.Target()->Position(), fTarget->Target()->Position())) {
		fActor.Target()->SetAnimationAction(ACT_WALKING);
		fActor.Target()->MoveToNextPointInPath(fActor.Target()->IsFlying());
	} else {
*/
		actor->SetAnimationAction(ACT_STANDING);
		fCompleted = true;
		actor->InitiateDialogWith(target);
	//}
}


// FadeToColor
FadeToColorAction::FadeToColorAction(Object* object, int32 numUpdates)
	:
	Action(object),
	fNumUpdates(numUpdates)
{
}


/* virtual */
void
FadeToColorAction::operator()()
{
}


// ChangeOrientationExtAction
ChangeOrientationExtAction::ChangeOrientationExtAction(Object* object, int o)
	:
	Action(object),
	fOrientation(o)
{
}


/* virtual */
void
ChangeOrientationExtAction::operator()()
{
	Actor* actor = dynamic_cast<Actor*>(fObject);
	if (actor != NULL)
		actor->SetOrientation(fOrientation);
}
