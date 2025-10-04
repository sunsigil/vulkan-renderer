#include "machines.h"

#include "timing.h"

// TOS_FSM
////////////////////////////////////////////////////////////////

TOS_FSM::TOS_FSM(TOS_FSM_state _state)
{
	state = _state;
	if(state != nullptr)
		state(TOS_FSM_SIGNAL_ENTER);
}

void TOS_FSM::transition(TOS_FSM_state _state)
{
	if(state != nullptr)
		state(TOS_FSM_SIGNAL_EXIT);
	state = _state;
	if(state != nullptr)
		state(TOS_FSM_SIGNAL_ENTER);
}

void TOS_FSM::tick()
{
	if(state != nullptr)
		state(TOS_FSM_SIGNAL_TICK);
}

bool TOS_FSM::is_live()
{
	return state != nullptr;
}


// TOS_latch
////////////////////////////////////////////////////////////////

TOS_latch::TOS_latch(bool state) : state(state), was(state) {}

bool TOS_latch::flip()
{
	state = !state;
	return state;
}

bool TOS_latch::flipped()
{
	return state != was;
}

void TOS_latch::tick()
{
	was = state;
}
