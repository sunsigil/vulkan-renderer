#pragma once

#include <chrono>

typedef std::chrono::time_point<std::chrono::high_resolution_clock> timepoint;

void TOS_create_timing_context();
void TOS_timing_tick();

uint64_t TOS_get_elapsed_frames();
float TOS_get_elapsed_time_s();
float TOS_get_delta_time_s();
int TOS_get_FPS();

class TOS_timeline
{
public:
	TOS_timeline(float duration=1.0f, bool reversed=false, bool paused=false);
	float get();
	float normalized();
	void set(float t);
	void pause();
	void resume();
	void reset();
	void reverse();
	void tick();
private:
	float duration;
	float t;
	bool reversed;
	bool paused;
};