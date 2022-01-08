#ifndef __ACTION_H
#define __ACTION_H

#include "IETypes.h"
#include "Reference.h"

class Actor;
class Door;
class Object;
struct action_params;
class Action {
public:
    Action(Object* actor, action_params* node);
    virtual ~Action();

    bool Initiated() const;
    void SetInitiated();
    bool Completed() const;
	void SetCompleted();
	
    virtual void operator()() = 0;
    
    bool IsInstant() const;
    std::string Name() const;
    
protected:
    Object* fSender;
    action_params* fActionParams;
private:
    bool fInitiated;
    bool fCompleted;
};


class ActionSetGlobal : public Action {
public:
	ActionSetGlobal(Object* object, action_params* action);
	virtual void operator()();
};


class ActionCreateCreature : public Action {
public:
	ActionCreateCreature(Object* object, action_params* action);
	virtual void operator()();
};


class ActionCreateCreatureImpassable : public Action {
public:
	ActionCreateCreatureImpassable(Object* object, action_params* node);
	virtual void operator()();
};


class ActionTriggerActivation : public Action {
public:
	ActionTriggerActivation(Object* object, action_params* node);
	virtual void operator()();
};


class ActionUnlock : public Action {
public:
	ActionUnlock(Object* object, action_params* node);
	virtual void operator()();
};


class ActionDestroySelf : public Action {
public:
	ActionDestroySelf(Object* object, action_params* node);
	virtual void operator()();
};


class ActionForceSpell : public Action {
public:
	ActionForceSpell(Object* object, action_params* node);
	virtual void operator()();
private:
	int fDuration;
	uint32 fStart;
};


class ActionForceSpellPoint : public Action {
public:
	ActionForceSpellPoint(Object* object, action_params* node);
	virtual void operator()();
private:
	int fDuration;
	uint32 fStart;
};


class ActionMoveBetweenAreasEffect : public Action {
public:
	ActionMoveBetweenAreasEffect(Object* object, action_params* node);
	virtual void operator()();
};


class ActionPlayDead : public Action {
public:
	ActionPlayDead(Object* object, action_params* node);
	virtual void operator()();
private:
	uint32 fDuration;
};


class ActionSetInterruptable : public Action {
public:
	ActionSetInterruptable(Object* object, action_params* node);
	virtual void operator()();
};


class ActionWalkTo : public Action {
public:
	ActionWalkTo(Object* actor, action_params* node, bool canInterrupt = true);
	virtual void operator()();
private:
	bool fInterruptable;
};


class ActionWalkToObject : public Action {
public:
	ActionWalkToObject(Object* actor, action_params* node);
	virtual void operator()();
};


class ActionRandomFly : public Action {
public:
	ActionRandomFly(Object* actor, action_params* node);
	virtual void operator()();
};


class ActionFlyTo : public Action {
public:
	ActionFlyTo(Object* actor, action_params* node);
	virtual void operator()();
private:
	IE::point fDestination;
};


class ActionIncrementGlobal : public Action {
public:
	ActionIncrementGlobal(Object* actor, action_params* node);
	virtual void operator()();
};


class ActionRandomWalk : public Action {
public:
	ActionRandomWalk(Object* actor, action_params* node);
	virtual void operator()();
};


class ActionWait : public Action {
public:
	ActionWait(Object* actor, action_params* node);
	virtual void operator()();
private:
	int32 fWaitTime;
};


class ActionSmallWait : public Action {
public:
	ActionSmallWait(Object* actor, action_params* node);
	virtual void operator()();
private:
	int32 fWaitTime;
};


class ActionOpenDoor : public Action {
public:
	ActionOpenDoor(Object* actor, action_params* node);
	virtual void operator()();
};

class ActionCloseDoor : public Action {
public:
	ActionCloseDoor(Object* actor, action_params* node);
	virtual void operator()();
};


class ActionDisplayMessage : public Action {
public:
	ActionDisplayMessage(Object* actor, action_params* node);
	virtual void operator()();
};


class ActionPlayMovie : public Action {
public:
	ActionPlayMovie(Object* object, action_params* node);
	virtual void operator()();
};


class ActionAttack : public Action {
public:
	ActionAttack(Object* actor, action_params* node);
	virtual void operator()();
};


class ActionRunAwayFrom : public Action {
public:
	ActionRunAwayFrom(Object* actor, action_params* node);
	virtual void operator()();
private:
	IE::point PointAway(Actor* actor, Actor* target);
};


class ActionDialog : public Action {
public:
	ActionDialog(Object* actor, action_params* node);
	virtual void operator()();
};


class ActionSetEnemyAlly : public Action {
public:
	ActionSetEnemyAlly(Object* actor, action_params* node);
	virtual void operator()();
};


class ActionFadeToColor : public Action {
public:
	ActionFadeToColor(Object* object, action_params* node);
	virtual void operator()();
private:
	int32 fCurrentValue;
	int32 fTargetValue;
	int16 fStepValue;
};


class ActionFadeFromColor : public Action {
public:
	ActionFadeFromColor(Object* object, action_params* node);
	virtual void operator()();
private:
	int32 fCurrentValue;
	int32 fTargetValue;
	int16 fStepValue;
};


class ActionMoveViewPoint : public Action {
public:
	ActionMoveViewPoint(Object* object, action_params* node);
	virtual void operator()();
private:
	IE::point fDestination;
	int fScrollSpeed;
};


class ActionStartTimer : public Action {
public:
	ActionStartTimer(Object* object, action_params* params);
	virtual void operator()();
};


class ActionScreenShake : public Action {
public:
	ActionScreenShake(Object* object, action_params* node);
	virtual void operator()();
private:
	IE::point fOffset;
	int fDuration;
};


class ActionStartCutsceneMode : public Action {
public:
	ActionStartCutsceneMode(Object* object, action_params* node);
	virtual void operator()();
};


class ActionEndCutsceneMode : public Action {
public:
	ActionEndCutsceneMode(Object* object, action_params* node);
	virtual void operator()();
};


class ActionClearAllActions : public Action {
public:
	ActionClearAllActions(Object* object, action_params* node);
	virtual void operator()();
};


class ActionSetGlobalTimer : public Action {
public:
	ActionSetGlobalTimer(Object* object, action_params* node);
	virtual void operator()();
};


class ActionStartCutscene : public Action {
public:
	ActionStartCutscene(Object* object, action_params* node);
	virtual void operator()();
};


class ActionHideGUI : public Action {
public:
	ActionHideGUI(Object* object, action_params* node);
	virtual void operator()();
};


class ActionUnhideGUI : public Action {
public:
	ActionUnhideGUI(Object* object, action_params* node);
	virtual void operator()();
};


class ActionDisplayString : public Action {
public:
	ActionDisplayString(Object* object, action_params* node);
	virtual void operator()();
private:
	std::string fString;
	IE::point fOffset;
	int fDuration;
};


class ActionDisplayStringHead : public Action {
public:
	ActionDisplayStringHead(Object* object, action_params* node);
	virtual void operator()();
private:
	uint32 fDuration;
};


class ActionChangeOrientationExt : public Action {
public:
	ActionChangeOrientationExt(Object* object, action_params* node);
	virtual void operator()();
};


class ActionFaceObject : public Action {
public:
	ActionFaceObject(Object* object, action_params* node);
	virtual void operator()();
};


class ActionCreateVisualEffect : public Action {
public:
	ActionCreateVisualEffect(Object* action, action_params* node);
	virtual void operator()();
};


class ActionCreateVisualEffectObject : public Action {
public:
	ActionCreateVisualEffectObject(Object* action, action_params* node);
	virtual void operator()();
};

#endif // __ACTION_H
