#ifndef __ACTION_H
#define __ACTION_H

#include "IETypes.h"

class Actor;
class Door;
class Action {
public:
    Action(Actor* actor);
    virtual ~Action();

    bool Initiated() const;
    bool Completed() const;

    virtual void Run();
protected:
    Actor* fActor;
    bool fInitiated;
    bool fCompleted;
};


class WalkTo : public Action {
public:
	WalkTo(Actor* actor, IE::point destination);
	virtual void Run();
private:
	IE::point fDestination;
};


class Wait : public Action {
public:
	Wait(Actor* actor, uint32 time);
	virtual void Run();
private:
	uint32 fWaitTime;
	uint32 fStartTime;
};


class Toggle : public Action {
public:
	// TODO: For any object ?
	Toggle(Actor* actor, Door* door);
	virtual void Run();
private:
	Door* fDoor;
};


class Attack : public Action {
public:
	Attack(Actor* actor, Actor* target);
	virtual void Run();
private:
	Actor* fTarget;
};


class RunAwayFrom : public Action {
public:
	RunAwayFrom(Actor* actor, Actor* target);
	virtual void Run();
private:
	Actor* fTarget;

	IE::point PointAway() const;
};
#endif
