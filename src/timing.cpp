#include "timing.h"

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