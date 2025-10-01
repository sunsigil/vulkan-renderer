#pragma once

#include <stdint.h>
#include <stddef.h>

struct TOS_PPM
{
	uint32_t width;
	uint32_t height;
	size_t size;

	uint8_t* pixels;
};

void TOS_PPM_load(TOS_PPM* ppm, const char* path);
void TOS_PPM_destroy(TOS_PPM* ppm);