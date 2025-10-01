#pragma once

#include <stdint.h>
#include <vector>

struct TOS_OBJ
{
	std::vector<float> v;
	std::vector<float> vt;
	std::vector<float> vn;
	std::vector<int> f;
};

void TOS_OBJ_load(TOS_OBJ* obj, const char* path);