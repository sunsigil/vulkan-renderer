#pragma once

#include <stdint.h>
#include <vector>
#include <map>

/*struct TOS_MTL
{
	const char* name;

	glm::vec3 ka;
	glm::vec3 kd;
	glm::vec3 ks;
	float ns;
	float d;
	float ni;

	const char* map_ka;
	const char* map_kd;
	const char* map_ks;
	const char* map_ns;
	const char* map_d;
};*/

struct TOS_OBJ
{
	//std::vector<TOS_MTL> mtl;
	std::vector<float> v;
	std::vector<float> vt;
	std::vector<float> vn;
	std::vector<int> f;
};

void TOS_OBJ_load(TOS_OBJ* obj, const char* path);