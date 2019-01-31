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
	uint32 fStartTime;
};


class Toggle : public Action {
public:
	// TODO: For any object ?
	Toggle(Object* actor, Door* door);
	virtual void operator()();
private:
	Reference<Door> fDoor;
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

class FadeToColorAction : public Action {
public:
	FadeToColorAction(Object* object, int32 numUpdates);
	virtual void operator()();
private:
	int32 fNumUpdates;
};

#endif
