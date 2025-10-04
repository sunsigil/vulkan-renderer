#include "transform.h"

#include "glm/gtx/transform.hpp"
#include <glm/gtx/quaternion.hpp>
#include "timing.h"

TOS_transform::TOS_transform()
{
	scale = glm::vec3(1);
	orientation = glm::vec3(0);
	position = glm::vec3(0);
	velocity = glm::vec3(0);
	acceleration = glm::vec3(0);
}

TOS_transform::TOS_transform
(
	glm::vec3 scale,
	glm::vec3 orientation,
	glm::vec3 position,
	glm::vec3 velocity,
	glm::vec3 acceleration
) :
scale(scale), orientation(orientation), position(position),
velocity(velocity), acceleration(acceleration) {}

glm::mat4 TOS_transform::M()
{
	glm::mat4 S = glm::scale(scale);
	glm::mat4 R = glm::toMat4(glm::quat(orientation));
	glm::mat4 T = glm::translate(position);
	return T*R*S;
}

glm::vec3 TOS_transform::forward()
{ 
	return glm::toMat4(glm::quat(orientation)) * glm::vec4(0,0,1,0);
}

glm::vec3 TOS_transform::up()
{
	return glm::toMat4(glm::quat(orientation)) * glm::vec4(0,1,0,0);
}

glm::vec3 TOS_transform::right()
{
	return glm::toMat4(glm::quat(orientation)) * glm::vec4(1,0,0,0);
}

void TOS_transform::tick()
{
	float dt = TOS_get_delta_time_s();
	velocity += acceleration * dt;
	position += velocity * dt;
}