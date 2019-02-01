#ifndef __ACTION_H
#define __ACTION_H

#include "IETypes.h"
#include "Reference.h"

class Door;
class Object;
class Action {
public:
    Action(Object* actor);
    virtual ~Action();

    bool Initiated() const;
    bool Completed() const;
	void SetCompleted();
	
    virtual void operator()();
protected:
    Object* fObject;
    bool fInitiated;
    bool fCompleted;
};


class ActionWithTarget : public Action {
public:
	ActionWithTarget(Object* actor, Object* target);
	virtual void operator()();

protected:
	Object* fTarget;
};


class SetInterruptableAction : public Action {
public:
	SetInterruptableAction(Object* object, bool interruptable);
	virtual void operator()();
private:
	bool fInterruptable;
};


class WalkTo : public Action {
public:
	WalkTo(Object* actor, IE::point destination);
	virtual void operator()();
private:
	IE::point fDestination;
};


class FlyTo : public Action {
public:
	FlyTo(Object* actor, IE::point, int time);
	virtual void operator()();
private:
	IE::point fDestination;
};


class Wait : public Action {
public:
	Wait(Object* actor, uint32 time);
	virtual void operator()();
private:
	uint32 fWaitTime;
};


class OpenDoor : public Action {
public:
	OpenDoor(Object* actor, Door* door);
	virtual void operator()();
private:
	Door* fDoor;
};


class Attack : public ActionWithTarget {
public:
	Attack(Object* actor, Object* target);
	virtual void operator()();
};


class RunAwayFrom : public ActionWithTarget {
public:
	RunAwayFrom(Object* actor, Object* target);
	virtual void operator()();
private:
	IE::point PointAway() const;
};


class Dialogue : public ActionWithTarget {
public:
	Dialogue(Object* actor, Object* target);
	virtual void operator()();
};


class FadeColorAction : public Action {
public:
	enum fade {
		FROM_BLACK = 1,
		TO_BLACK = -1
	};
	FadeColorAction(Object* object, int32 numUpdates, int fade);
	virtual void operator()();
private:
	int32 fNumUpdates;
	int fFadeDirection;
	int32 fCurrentValue;
	int32 fTargetValue;
	int16 fStepValue;
};


class MoveViewPoint : public Action {
public:
	MoveViewPoint(Object* object, IE::point point, int scrollSpeed);
	virtual void operator()();
private:
	IE::point fDestination;
	int fScrollSpeed;
};


class ChangeOrientationExtAction : public Action {
public:
	ChangeOrientationExtAction(Object* object, int o);
	virtual void operator()();
private:
	int fOrientation;
};
#endif
