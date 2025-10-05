#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "transform.h"
#include "geometry.h"

class TOS_camera
{
public:
	TOS_transform transform;
	float fov;
	float aspect;
	float near;
	float far;

	TOS_camera();
	TOS_camera
	(
		glm::vec3 position, glm::vec3 orientation,
		float fov, float aspect, float near, float far
	);

	void rotate(float pitch, float yaw);
	glm::mat4 V();
	glm::mat4 P();
	TOS_ray viewport_ray(float x, float y);
	void tick();
};