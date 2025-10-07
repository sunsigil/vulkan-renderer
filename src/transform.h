#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

enum TOS_axis
{
	TOS_AXIS_X,
	TOS_AXIS_Y,
	TOS_AXIS_Z,
	TOS_AXIS_COUNT
};

enum TOS_transform_op
{
	TOS_TRANSFORM_OP_TRANSLATE,
	TOS_TRANSFORM_OP_ROTATE,
	TOS_TRANSFORM_OP_SCALE,
	TOS_TRANSFORM_OP_COUNT
};

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