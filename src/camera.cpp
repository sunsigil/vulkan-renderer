#include "camera.h"

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
	return glm::perspective(fov, aspect, near, far);
}

void TOS_camera::tick()
{
	transform.tick();
}
