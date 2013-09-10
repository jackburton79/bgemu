#include "Action.h"
#include "Actor.h"
#include "Animation.h"
#include "Door.h"
#include "Timer.h"

Action::Action(Actor* actor)
    :
	fActor(actor),
	fCompleted(false)
{
}


Action::~Action()
{

}


bool
Action::Completed() const
{
    return fCompleted;
}


/* virtual */
void
Action::Run()
{
}


// WalkTo
WalkTo::WalkTo(Actor* actor, IE::point destination)
	:
	Action(actor),
	fDestination(destination)
{
	if (fActor->Destination() != fDestination)
		fActor->SetDestination(fDestination);
}


/* virtual */
void
WalkTo::Run()
{


	if (fActor->Position() == fActor->Destination()) {
		fCompleted = true;
		fActor->SetAnimationAction(ACT_STANDING);
		return;
	}

	fActor->SetAnimationAction(ACT_WALKING);
	fActor->UpdatePath(fActor->IsFlying());
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
Wait::Run()
{
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
Toggle::Run()
{
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
Attack::Run()
{
	fActor->SetAnimationAction(ACT_ATTACKING);
	fActor->AttackTarget(fTarget);
}
