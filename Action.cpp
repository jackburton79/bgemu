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


Action::Action(Actor* actor)
    :
	fActor(actor),
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


// WalkTo
WalkTo::WalkTo(Actor* actor, IE::point destination)
	:
	Action(actor),
	fDestination(destination)
{
}


/* virtual */
void
WalkTo::operator()()
{
	if (!Initiated())
		fActor.Target()->SetDestination(fDestination);

	Action::operator()();

	if (fActor.Target()->Position() == fActor.Target()->Destination()) {
		fCompleted = true;
		fActor.Target()->SetAnimationAction(ACT_STANDING);
		return;
	}

	fActor.Target()->SetAnimationAction(ACT_WALKING);
	fActor.Target()->MoveToNextPointInPath(fActor.Target()->IsFlying());
}


// FlyTo
FlyTo::FlyTo(Actor* actor, IE::point destination, int time)
	:
	Action(actor),
	fDestination(destination)
{
}


/* virtual */
void
FlyTo::operator()()
{
	if (!Initiated())
		fActor.Target()->SetDestination(fDestination);

	Action::operator()();

	if (fActor.Target()->Position() == fActor.Target()->Destination()) {
		fCompleted = true;
		fActor.Target()->SetAnimationAction(ACT_STANDING);
		return;
	}

	fActor.Target()->SetAnimationAction(ACT_WALKING);
	fActor.Target()->MoveToNextPointInPath(true);
}


// Wait
Wait::Wait(Actor* actor, uint32 time)
	:
	Action(actor),
	fWaitTime(time)
{
	fStartTime = Timer::GameTime();
}


void
Wait::operator()()
{
	Action::operator()();
	if (Timer::GameTime() >= fStartTime + fWaitTime)
		fCompleted = true;
}


// Toggle
Toggle::Toggle(Actor* actor, Door* door)
	:
	Action(actor),
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
Attack::Attack(Actor* actor, Actor* target)
	:
	Action(actor),
	fTarget(target)
{
}


/* virtual */
void
Attack::operator()()
{
	if (!Initiated()) {
		IE::point point = fTarget.Target()->NearestPoint(fActor.Target()->Position());
		if (!PointSufficientlyClose(fActor.Target()->Position(), point))
			fActor.Target()->SetDestination(point);
		Action::operator()();
	}

	if (fActor.Target()->Position() != fActor.Target()->Destination()) {
		fActor.Target()->SetAnimationAction(ACT_WALKING);
		fActor.Target()->MoveToNextPointInPath(fActor.Target()->IsFlying());
	} else {
		fActor.Target()->SetAnimationAction(ACT_ATTACKING);
		fActor.Target()->AttackTarget(fTarget.Target());
		fCompleted = true;
	}
}


// RunAwayFrom
RunAwayFrom::RunAwayFrom(Actor* actor, Actor* target)
	:
	Action(actor),
	fTarget(target)
{

}


/* virtual */
void
RunAwayFrom::operator()()
{
	Action::operator()();

	// TODO: Improve.
	// TODO: We are recalculating this every time. Is it correct ?
	if (Core::Get()->Distance(fActor.Target(), fTarget.Target()) < 200) {
		IE::point point = PointAway();
		if (fActor.Target()->Destination() != point) {
			fActor.Target()->SetDestination(point);
		}
	}

	if (fActor.Target()->Position() == fActor.Target()->Destination()) {
		fCompleted = true;
		fActor.Target()->SetAnimationAction(ACT_STANDING);
	} else {
		fActor.Target()->SetAnimationAction(ACT_WALKING);
		fActor.Target()->MoveToNextPointInPath(fActor.Target()->IsFlying());
	}
}


IE::point
RunAwayFrom::PointAway() const
{
	IE::point point = fActor.Target()->Position();

	if (fTarget.Target()->Position().x > fActor.Target()->Position().x)
		point.x -= 100;
	else if (fTarget.Target()->Position().x < fActor.Target()->Position().x)
		point.x += 100;

	if (fTarget.Target()->Position().y > fActor.Target()->Position().y)
		point.y -= 100;
	else if (fTarget.Target()->Position().y < fActor.Target()->Position().y)
		point.y += 100;

	return point;
}


// Dialogue
Dialogue::Dialogue(Actor* source, Actor* target)
	:
	Action(source),
	fTarget(target)
{

}


/* virtual */
void
Dialogue::operator()()
{
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
		fActor.Target()->SetAnimationAction(ACT_STANDING);
		fCompleted = true;
		fActor.Target()->InitiateDialogWith(fTarget.Target());
	//}
}
