#include "timing.h"

#include "cowtools.h"

static uint64_t frames;
static timepoint start;
static timepoint now;
static float dt;

void TOS_create_timing_context()
{
	frames = 0;
	start = std::chrono::high_resolution_clock::now();
	now = std::chrono::high_resolution_clock::now();
	dt = 0;
}

void TOS_timing_tick()
{
	frames += 1;
	timepoint then = now;
	now = std::chrono::high_resolution_clock::now();
	dt = std::chrono::duration<float, std::chrono::seconds::period>(now - then).count();
}

uint64_t TOS_get_elapsed_frames()
{
	return frames;
}

float TOS_get_elapsed_time_s()
{
	return std::chrono::duration<float, std::chrono::seconds::period>(now - start).count();
}

float TOS_get_delta_time_s()
{
	return dt;
}

int TOS_get_FPS()
{
	return round(1.0f/dt);
}


// TOS_timeline
////////////////////////////////////////////////////////////////

TOS_timeline::TOS_timeline(float duration, bool reversed, bool paused) :
duration(duration), t(0), reversed(reversed), paused(paused) {}

float TOS_timeline::get()
{
	if(reversed)
		return duration - t;
	return t;
}

float TOS_timeline::normalized()
{
	return get() / duration;
}

void TOS_timeline::set(float _t)
{
	t = TOS_clamp(_t, 0, duration);
}

void TOS_timeline::pause()
{
	paused = true;
}

void TOS_timeline::resume()
{
	paused = false;
}

void TOS_timeline::reset()
{
	t = 0;
}

void TOS_timeline::reverse()
{
	reversed = !reversed;
}

void TOS_timeline::tick()
{
	if(!paused)
		t = TOS_clamp(t + TOS_get_delta_time_s(), 0, duration);
}