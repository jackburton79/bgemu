#ifndef __ACTION_H
#define __ACTION_H

#include "Actor.h"
#include "IETypes.h"
#include "Reference.h"

class Actor;
class Door;
class Action {
public:
    Action(Actor* actor);
    virtual ~Action();

    bool Initiated() const;
    bool Completed() const;

    virtual void operator()();
protected:
    Reference<Actor> fActor;
    bool fInitiated;
    bool fCompleted;
};


class ActionWithTarget : public Action {
public:
	ActionWithTarget(Actor* actor, Actor* target);
	virtual void operator()();

protected:
	Reference<Actor> fTarget;
};


class WalkTo : public Action {
public:
	WalkTo(Actor* actor, IE::point destination);
	virtual void operator()();
private:
	IE::point fDestination;
};


class FlyTo : public Action {
public:
	FlyTo(Actor* actor, IE::point, int time);
	virtual void operator()();
private:
	IE::point fDestination;
};


class Wait : public Action {
public:
	Wait(Actor* actor, uint32 time);
	virtual void operator()();
private:
	uint32 fWaitTime;
	uint32 fStartTime;
};


class Toggle : public Action {
public:
	// TODO: For any object ?
	Toggle(Actor* actor, Door* door);
	virtual void operator()();
private:
	Reference<Door> fDoor;
};


class Attack : public ActionWithTarget {
public:
	Attack(Actor* actor, Actor* target);
	virtual void operator()();
};


class RunAwayFrom : public ActionWithTarget {
public:
	RunAwayFrom(Actor* actor, Actor* target);
	virtual void operator()();
private:
	IE::point PointAway() const;
};


class Dialogue : public ActionWithTarget {
public:
	Dialogue(Actor* actor, Actor* target);
	virtual void operator()();
};
#endif
