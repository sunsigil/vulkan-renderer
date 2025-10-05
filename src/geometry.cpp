#include "geometry.h"

#include "cowtools.h"
#include <iostream>

TOS_AABB TOS_AABB::min_max(glm::vec3 min, glm::vec3 max)
{
	return
	TOS_AABB
	{
		.min = min,
		.max = max
	};
}

TOS_AABB TOS_AABB::center_size(glm::vec3 center, glm::vec3 size)
{
	return
	TOS_AABB
	{
		.min = center - size * 0.5f,
		.max = center + size * 0.5f
	};
}

TOS_AABB TOS_AABB::transform(glm::mat4 T)
{
	TOS_AABB aabb;
	glm::vec4 m = T * glm::vec4(min, 1);
	glm::vec4 M = T * glm::vec4(max, 1);
	aabb.min = glm::vec3(m/m.w);
	aabb.max = glm::vec3(M/M.w);
	return aabb;
}

TOS_ray TOS_ray::direction_magnitude(glm::vec3 origin, glm::vec3 direction, float magnitude)
{
	return
	TOS_ray
	{
		.origin = origin,
		.direction = glm::normalize(direction),
		.t = magnitude
	};
}

TOS_ray TOS_ray::arrow(glm::vec3 origin, glm::vec3 arrow)
{
	return
	TOS_ray
	{
		.origin = origin,
		.direction = glm::normalize(arrow),
		.t = glm::length(arrow)
	};
}

// Real Time Collision Detection, Christer Ericson
static int intersect_ray_AABB(glm::vec3 p, glm::vec3 d, TOS_AABB a, float& tmin, glm::vec3& q)
{
	tmin = 0.0f;
	float tmax = FLT_MAX;
	
	for(int i = 0; i < 3; i++)
	{
		if(abs(d[i]) < FLT_EPSILON)
		{
			if(p[i] < a.min[i] || p[i] > a.max[i])
				return 0;
		}
		else
		{
			float ood = 1.0f / d[i];
			float t1 = (a.min[i] - p[i]) * ood;
			float t2 = (a.max[i] - p[i]) * ood;
			if(t1 > t2)
			{
				float temp = t1;
				t1 = t2;
				t2 = temp;
			}
			tmin = TOS_max(tmin, t1);
			tmax = TOS_min(tmax, t2);
			if(tmin > tmax)
				return 0;
		}
	}
	q = p + d * tmin;
	return 1;
}

std::optional<TOS_raycast_hit> TOS_ray_AABB_intersect(TOS_ray ray, TOS_AABB aabb)
{
	std::optional<TOS_raycast_hit> hit;
	float tmin;
	glm::vec3 q;
	if(intersect_ray_AABB(ray.origin, ray.direction, aabb, tmin, q))
	{
		hit = TOS_raycast_hit
		{
			.point = q,
			.normal = glm::vec3(0),
			.t = tmin
		};
	}
	return hit;
}

static int intersect_ray_OBB(glm::vec3 p, glm::vec3 d, TOS_AABB a, glm::mat4 T, float& tmin, glm::vec3& q)
{
	tmin = 0.0f;
	float tmax = FLT_MAX;
	glm::vec3 world_pos(T[3].x, T[3].y, T[3].z);
	glm::vec3 delta = world_pos - p;
	
	for(int i = 0; i < 3; i++)
	{
		if(abs(d[i]) < FLT_EPSILON)
		{
			if(p[i] < a.min[i] || p[i] > a.max[i])
				return 0;
		}
		else
		{
			glm::vec3 axis(T[i].x, T[i].y, T[i].z);
			float e = glm::dot(axis, delta);
			float f = glm::dot(d, axis);

			float t1 = (e+a.min[i])/f;
			float t2 = (e+a.max[i])/f;
			if(t1 > t2)
			{
				float temp = t1;
				t1 = t2;
				t2 = temp;
			}

			tmax = TOS_min(tmax, t2);
			tmin = TOS_max(tmin, t1);
			if(tmax < tmin)
				return 0;
		}
	}
	q = p + d * tmin;
	return 1;
}

std::optional<TOS_raycast_hit> TOS_ray_OBB_intersect(TOS_ray ray, TOS_AABB aabb, glm::mat4 T)
{
	std::optional<TOS_raycast_hit> hit;
	float tmin;
	glm::vec3 q;
	if(intersect_ray_OBB(ray.origin, ray.direction, aabb, T, tmin, q))
	{
		hit = TOS_raycast_hit
		{
			.point = q,
			.normal = glm::vec3(0),
			.t = tmin
		};
	}
	return hit;
}