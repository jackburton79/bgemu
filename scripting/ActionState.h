#pragma once

#include "IETypes.h"

#include <string>

class Action;

// Runtime state for whichever action an Object is currently executing.
//
// An Object only ever executes one action at a time (fCurrentActionParams);
// actions still waiting in the queue are plain action_params* with no state
// of their own. That means a single flat struct, reset each time a new
// action becomes current, is enough - no per-action-type union/variant is
// needed even though different action types use these fields for different
// things (e.g. `counter` is the tick countdown for Wait, SmallWait, PlayDead
// and ForceSpell alike).
struct action_state {
	bool initiated = false;
	bool completed = false;

	bool flag = false;        // e.g. ActionWalkTo's "interruptable"
	int16 step = 0;            // e.g. FadeToColor/FadeFromColor's step
	int32 counter = 0;          // generic countdown (Wait, SmallWait, PlayDead,
	                             // ForceSpell duration, ScreenShake, ...)
	int32 extra = 0;             // secondary value (fade target, scroll speed, ...)
	uint32 startTick = 0;         // ForceSpell/ForceSpellPoint start timestamp
	                               // (diagnostics only)
	IE::point point{};              // destination/offset
	std::string text;                // DisplayString's text

};
