#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

class TOS_transform
{
public:
	glm::vec3 scale;
	glm::vec3 orientation;
	glm::vec3 position;
	glm::vec3 velocity;
	glm::vec3 acceleration;

	TOS_transform();
	TOS_transform
	(
		glm::vec3 scale,
		glm::vec3 orientation,
		glm::vec3 position,
		glm::vec3 velocity,
		glm::vec3 acceleration
	);

	glm::mat4 M();
	glm::vec3 forward();
	glm::vec3 up();
	glm::vec3 right();

	void tick();
};