#include "camera.h"

#include "glm/gtx/string_cast.hpp"
#include <iostream>

TOS_camera::TOS_camera() {}

TOS_camera::TOS_camera
(
	glm::vec3 position, glm::vec3 orientation,
	float fov, float aspect, float near, float far
) :
fov(fov), aspect(aspect), near(near), far(far)
{
	transform = TOS_transform();
	transform.position = position;
	transform.orientation = orientation;

	P_cached = glm::perspective(fov, aspect, near, far);
	P_inv_cached = glm::inverse(P_cached);
	R = glm::inverse(V()) * P_inv_cached;
}

void TOS_camera::rotate(float pitch, float yaw)
{
	transform.orientation.x += pitch;
	transform.orientation.y += yaw;
}

glm::mat4 TOS_camera::V()
{
	return glm::lookAt(transform.position, transform.position + transform.forward(), transform.up());
}

glm::mat4 TOS_camera::P()
{
	return P_cached;
}

TOS_ray TOS_camera::viewport_ray(float x, float y)
{
	glm::vec4 ndc = glm::vec4(2*x-1, 2*y-1, 1, 1);
	ndc.y *= -1;
	glm::vec4 world = R * ndc;
	world /= world.w;
	return TOS_ray::direction_magnitude(transform.position, glm::vec3(world)-transform.position, far);
}

void TOS_camera::tick()
{
	transform.tick();

	R = glm::inverse(V()) * P_inv_cached;
}
