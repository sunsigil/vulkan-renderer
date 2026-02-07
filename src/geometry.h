#pragma once

#include "glm/glm.hpp"
#include <optional>

struct TOS_AABB
{
	static TOS_AABB min_max(glm::vec3 min, glm::vec3 max);
	static TOS_AABB center_size(glm::vec3 center, glm::vec3 size);

	glm::vec3 min;
	glm::vec3 max;

	TOS_AABB transform(glm::mat4 T);
};

struct TOS_plane
{
	static TOS_plane three_points(glm::vec3 a, glm::vec3 b, glm::vec3 c);

	glm::vec3 normal;
	float d;
};

struct TOS_segment
{
	glm::vec3 a;
	glm::vec3 b;
};

struct TOS_ray
{
	glm::vec3 origin;
	glm::vec3 direction;
	float t;

	static TOS_ray direction_magnitude(glm::vec3 origin, glm::vec3 direction, float magnitude);
	static TOS_ray arrow(glm::vec3 origin, glm::vec3 arrow);
};

struct TOS_sphere
{
	glm::vec3 center;
	float r;

	static TOS_sphere center_radius(glm::vec3 center, float r);
};

struct TOS_raycast_hit
{
	glm::vec3 point;
	glm::vec3 normal;
	float t;
};

std::optional<TOS_raycast_hit> TOS_ray_AABB_intersect(TOS_ray ray, TOS_AABB aabb);
std::optional<TOS_raycast_hit> TOS_ray_OBB_intersect(TOS_ray ray, TOS_AABB aabb, glm::mat4 T);
std::optional<TOS_raycast_hit> TOS_ray_plane_intersect(TOS_ray ray, TOS_plane plane);
float TOS_ray_segment_nearest(TOS_ray ray, TOS_segment segment, glm::vec3* ray_pt=nullptr, glm::vec3* segment_pt=nullptr);
std::optional<TOS_raycast_hit> TOS_ray_sphere_intersect(TOS_ray ray, TOS_sphere sphere);