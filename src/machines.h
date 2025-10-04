#pragma once

#include "timing.h"

enum TOS_FSM_signal
{
	TOS_FSM_SIGNAL_ENTER,
	TOS_FSM_SIGNAL_TICK,
	TOS_FSM_SIGNAL_EXIT
};

typedef void (*TOS_FSM_state) (TOS_FSM_signal);

class TOS_FSM
{
public:
	TOS_FSM(TOS_FSM_state state);
	void transition(TOS_FSM_state state);
	bool is_live();
	void tick();
private:
	TOS_FSM_state state;
};

class TOS_latch
{
public:
	bool state;

	TOS_latch(bool state);
	bool flip();
	bool flipped();
	void tick();
private:
	bool was;
};