#include "Action.h"
#include "Actor.h"
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
		return;
	}

	// TODO: Replicate/Move the logic in Actor
	fActor->UpdateMove(fActor->IsFlying());
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
