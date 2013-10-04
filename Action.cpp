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
		fActor->SetDestination(fDestination);

	Action::operator()();

	if (fActor->Position() == fActor->Destination()) {
		fCompleted = true;
		fActor->SetAnimationAction(ACT_STANDING);
		return;
	}

	fActor->SetAnimationAction(ACT_WALKING);
	fActor->MoveToNextPointInPath(fActor->IsFlying());
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
	fDoor->Toggle();
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
	Action::operator()();
	fActor->SetAnimationAction(ACT_ATTACKING);
	fActor->AttackTarget(fTarget);
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
	if (Core::Get()->Distance(fActor, fTarget) < 200) {
		IE::point point = PointAway();
		if (fActor->Destination() != point) {
			fActor->SetDestination(point);
		}
	}

	if (fActor->Position() == fActor->Destination()) {
		fCompleted = true;
		fActor->SetAnimationAction(ACT_STANDING);
		return;
	}

	fActor->SetAnimationAction(ACT_WALKING);
	fActor->MoveToNextPointInPath(fActor->IsFlying());
}


IE::point
RunAwayFrom::PointAway() const
{
	IE::point point = fActor->Position();

	if (fTarget->Position().x > fActor->Position().x)
		point.x -= 100;
	else if (fTarget->Position().x < fActor->Position().x)
		point.x += 100;

	if (fTarget->Position().y > fActor->Position().y)
		point.y -= 100;
	else if (fTarget->Position().x < fActor->Position().y)
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
	if (!PointSufficientlyClose(fActor->Destination(), fTarget->Position()))
		fActor->SetDestination(fTarget->Position());

	Action::operator()();

	if (!PointSufficientlyClose(fActor->Position(), fTarget->Position())) {
		fActor->SetAnimationAction(ACT_WALKING);
		fActor->MoveToNextPointInPath(fActor->IsFlying());
	} else {
		fActor->SetAnimationAction(ACT_STANDING);
		// TODO: Initiate the dialog
		fCompleted = true;
	}
}
