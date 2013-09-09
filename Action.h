#ifndef __ACTION_H
#define __ACTION_H

#include "IETypes.h"

class Actor;
class Action {
public:
    Action(Actor* actor);
    bool Completed() const;
    virtual void Run();
protected:
    Actor* fActor;
    bool fCompleted;
};


class WalkTo : public Action {
public:
	WalkTo(Actor* actor, IE::point destination);
	virtual void Run();
private:
	IE::point fDestination;
};

#endif
