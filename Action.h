#ifndef __ACTION_H
#define __ACTION_H

#include "IETypes.h"
#include "Reference.h"

class Door;
class Object;
struct action_node;
class Action {
public:
    Action(Object* actor, action_node* node);
    virtual ~Action();

    bool Initiated() const;
    void SetInitiated();
    bool Completed() const;
	void SetCompleted();
	
    virtual void operator()() = 0;
    
    std::string Name() const;
    
protected:
    Object* fObject;
    action_node* fActionParams;
private:
    bool fInitiated;
    bool fCompleted;
};


class SetInterruptableAction : public Action {
public:
	SetInterruptableAction(Object* object, action_node* node);
	virtual void operator()();
};


class WalkTo : public Action {
public:
	WalkTo(Object* actor, action_node* node);
	virtual void operator()();
private:
	IE::point fDestination;
};


class WalkToObject : public Action {
public:
	WalkToObject(Object* actor, action_node* node);
	virtual void operator()();
private:
	Object* fTarget;
};


class FlyTo : public Action {
public:
	FlyTo(Object* actor, action_node* node);
	virtual void operator()();
private:
	IE::point fDestination;
};


class Wait : public Action {
public:
	Wait(Object* actor, action_node* node);
	virtual void operator()();
private:
	uint32 fWaitTime;
};


class SmallWait : public Action {
public:
	SmallWait(Object* actor, action_node* node);
	virtual void operator()();
private:
	uint32 fWaitTime;
};


class OpenDoor : public Action {
public:
	OpenDoor(Object* actor, action_node* node);
	virtual void operator()();
private:
	Door* fDoor;
};


class Attack : public Action {
public:
	Attack(Object* actor, action_node* node);
	virtual void operator()();
};


class RunAwayFrom : public Action {
public:
	RunAwayFrom(Object* actor, action_node* node);
	virtual void operator()();
private:
	IE::point PointAway();
};


class Dialogue : public Action {
public:
	Dialogue(Object* actor, action_node* node);
	virtual void operator()();
};


class FadeToColorAction : public Action {
public:
	FadeToColorAction(Object* object, action_node* node);
	virtual void operator()();
private:
	int32 fNumUpdates;
	int32 fCurrentValue;
	int32 fTargetValue;
	int16 fStepValue;
};

class FadeFromColorAction : public Action {
public:
	FadeFromColorAction(Object* object, action_node* node);
	virtual void operator()();
private:
	int32 fNumUpdates;
	int32 fCurrentValue;
	int32 fTargetValue;
	int16 fStepValue;
};

class MoveViewPoint : public Action {
public:
	MoveViewPoint(Object* object, action_node* node);
	virtual void operator()();
private:
	IE::point fDestination;
	int fScrollSpeed;
};


class ScreenShake : public Action {
public:
	ScreenShake(Object* object, action_node* node);
	virtual void operator()();
private:
	IE::point fOffset;
	int fDuration;
};


class DisplayString : public Action {
public:
	DisplayString(Object* object, action_node* node);
	virtual void operator()();
private:
	std::string fString;
	IE::point fOffset;
	int fDuration;
};


class ChangeOrientationExtAction : public Action {
public:
	ChangeOrientationExtAction(Object* object, action_node* node);
	virtual void operator()();
private:
	int fOrientation;
};

#endif
