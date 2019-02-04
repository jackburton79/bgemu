#include "Action.h"
#include "Actor.h"
#include "Animation.h"
#include "Core.h"
#include "Door.h"
#include "GraphicsEngine.h"
#include "RoomBase.h"
#include "TextSupport.h"
#include "Timer.h"

#include <algorithm>
#include <typeinfo>

static bool
PointSufficientlyClose(const IE::point& pointA, const IE::point& pointB)
{
	return (std::abs(pointA.x - pointB.x) <= 5 * 2)
		&& (std::abs(pointA.y - pointB.y) <= 5 * 2);
}

// TODO: we should not pass Object pointers,
// but pass action parameters instead, which should be evaluated
// when the action is being executed
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


void
Action::SetCompleted()
{
	fCompleted = true;
}


/* virtual */
void
Action::operator()()
{
	if (!fInitiated)
		fInitiated = true;
}


std::string
Action::Name() const
{
	return typeid(this).name();
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


// SetInterruptableAction
SetInterruptableAction::SetInterruptableAction(Object* object, bool interrupt)
	:
	Action(object),
	fInterruptable(interrupt)
{
}


/* virtual */
void
SetInterruptableAction::operator()()
{
	fObject->SetInterruptable(fInterruptable);
	SetCompleted();
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
		SetCompleted();
		actor->SetAnimationAction(ACT_STANDING);
		return;
	}

	actor->SetAnimationAction(ACT_WALKING);
	actor->MoveToNextPointInPath(actor->IsFlying());
}


// WalkToObject
WalkToObject::WalkToObject(Object* object, Object* target)
	:
	Action(object),
	fTarget(target)
{
	fTarget->Acquire();
}


/* virtual */
void
WalkToObject::operator()()
{
	Actor* actor = dynamic_cast<Actor*>(fObject);
	if (actor == NULL)
		return;

	IE::point destination = fTarget->NearestPoint(actor->Position());
	actor->SetDestination(destination);

	Action::operator()();

	if (actor->Position() == actor->Destination()) {
		SetCompleted();
		actor->SetAnimationAction(ACT_STANDING);
		fTarget->Release();
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
		SetCompleted();
		actor->SetAnimationAction(ACT_STANDING);
		return;
	}

	actor->SetAnimationAction(ACT_WALKING);
	actor->MoveToNextPointInPath(true);
}


// Wait
Wait::Wait(Object* object, uint32 time)
	:
	Action(object),
	fWaitTime(time)
{
}


/* virtual */
void
Wait::operator()()
{
	fObject->SetWaitTime(fWaitTime);
	SetCompleted();
}


// OpenDoor
OpenDoor::OpenDoor(Object* sender, Door* door)
	:
	Action(sender),
	fDoor(door)
{
}


/* virtual */
void
OpenDoor::operator()()
{
	Action::operator()();
	if (!fDoor->Opened())
		fDoor->Toggle();
	SetCompleted();
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
		IE::point point = target->NearestPoint(actor->Position());
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
		SetCompleted();
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
		SetCompleted();
		actor->SetAnimationAction(ACT_STANDING);
	} else {
		actor->SetAnimationAction(ACT_WALKING);
		actor->MoveToNextPointInPath(actor->IsFlying());
	}
}


IE::point
RunAwayFrom::PointAway() const
{
	Actor* actor = dynamic_cast<Actor*>(fObject);
	if (actor == NULL) {
		return (IE::point ){ 0, 0 };
	}
	
	IE::point targetPos = fTarget->NearestPoint(actor->Position());

	if (targetPos.x > actor->Position().x)
		targetPos.x -= 100;
	else if (targetPos.x < actor->Position().x)
		targetPos.x += 100;

	if (targetPos.y > actor->Position().y)
		targetPos.y -= 100;
	else if (targetPos.y < actor->Position().y)
		targetPos.y += 100;

	return targetPos;
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
		SetCompleted();
		actor->InitiateDialogWith(target);
	//}
}


// FadeToColor
FadeColorAction::FadeColorAction(Object* object,
	int32 numUpdates, int fadeDirection)
	:
	Action(object),
	fFadeDirection(fadeDirection),
	fCurrentValue(0),
	fTargetValue(0),
	fStepValue(1)
{
	if (fFadeDirection == TO_BLACK) {
		fCurrentValue = 255;
		fTargetValue = 0;
		fStepValue = (fCurrentValue - fTargetValue) / numUpdates;
	} else if (fFadeDirection == FROM_BLACK) {
		fCurrentValue = 0;
		fTargetValue = 255;
		fStepValue = fTargetValue / numUpdates;
	}
	if (object != NULL)
		object->SetWaitTime(std::abs(fStepValue * numUpdates));
}


/* virtual */
void
FadeColorAction::operator()()
{
	GraphicsEngine::Get()->SetFade(fCurrentValue);
	switch (fFadeDirection) {
		case TO_BLACK:
			if (fCurrentValue > fTargetValue)
				fCurrentValue -= fStepValue;
			else
				SetCompleted();
			break;
		case FROM_BLACK:
			if (fCurrentValue < fTargetValue)
				fCurrentValue += fStepValue;
			else
				SetCompleted();
			break;
		default:
			SetCompleted();
			break;
	}
}


// MoveViewPoint
MoveViewPoint::MoveViewPoint(Object* object, IE::point point, int scrollSpeed)
	:
	Action(object),
	fDestination(point),
	fScrollSpeed(scrollSpeed)
{
	Core::Get()->CurrentRoom()->SanitizeOffsetCenter(fDestination);
	
	switch (fScrollSpeed) {
		case 1:
			fScrollSpeed = 10;
			break;
		case 2:
			fScrollSpeed = 20;
			break;
		case 3:
			fScrollSpeed = 40;
			break;
		case 4:
			fScrollSpeed = 80;
			break;
		case 0:		
		default:		
			fScrollSpeed = 10000;
			break;
	}
}


/* virtual */
void
MoveViewPoint::operator()()
{
	RoomBase* room = Core::Get()->CurrentRoom();
	IE::point offset = room->AreaCenterPoint();
	const int16 step = fScrollSpeed;
	if (offset != fDestination) {
		if (offset.x > fDestination.x)
			offset.x = std::max((int16)(offset.x - step), fDestination.x);
		else if (offset.x < fDestination.x)
			offset.x = std::min((int16)(offset.x + step), fDestination.x);
		
		if (offset.y > fDestination.y)
			offset.y = std::max((int16)(offset.y - step), fDestination.y);
		else if (offset.y < fDestination.y)
			offset.y = std::min((int16)(offset.y + step), fDestination.y);
		room->SetAreaOffsetCenter(offset);
	} else
		SetCompleted();
}


ScreenShake::ScreenShake(Object* object, IE::point point, int duration)
	:
	Action(object),
	fOffset(point),
	fDuration(duration)
{
	if (object != NULL)
		object->SetWaitTime(fDuration);
}


/* virtual */
void
ScreenShake::operator()()
{
	GFX::point point = { 0, 0 };
	if (fDuration-- == 0) {	
		GraphicsEngine::Get()->SetRenderingOffset(point);	
		SetCompleted();
		return;
	}
	
	point.x = fOffset.x;	
	point.y = fOffset.y;
	
	GraphicsEngine::Get()->SetRenderingOffset(point);
	fOffset.x = -fOffset.x;
	fOffset.y = -fOffset.y;
}



DisplayString::DisplayString(Object* object, const char* text, IE::point point, int duration)
	:
	Action(object),
	fString(text),
	fOffset(point),
	fDuration(duration)
{
	std::cout << "DisplayString " << text << " at " << fOffset.x << ", " << fOffset.y;
	std::cout << ", duration: " << duration << std::endl;
}


/* virtual */
void
DisplayString::operator()()
{
	if (fDuration-- == 0) {	
		SetCompleted();
		return;
	}
	
	GFX::point point = { fOffset.x, fOffset.y };
	FontRoster::GetFont("TOOLFONT")->RenderString(fString, 0, GraphicsEngine::Get()->ScreenBitmap(), point);
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
	SetCompleted();
}
