// Many routines here are taken from the book
// Real-Time Collision Detection, by Christer Ericson.

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

TOS_plane TOS_plane::three_points(glm::vec3 a, glm::vec3 b, glm::vec3 c)
{
	TOS_plane p;
	p.normal = glm::normalize(glm::cross(b-a, c-a));
	p.d = glm::dot(p.normal, a);
	return p;
}

TOS_sphere TOS_sphere::center_radius(glm::vec3 center, float r)
{
	TOS_sphere s;
	s.center = center;
	s.r = r;
	return s;
}

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

int intersect_segment_plane(glm::vec3 a, glm::vec3 b, TOS_plane p, float& t, glm::vec3& q)
{
	glm::vec3 ab = b-a;
	t = (p.d - glm::dot(p.normal, a)) / glm::dot(p.normal, ab);
	if(t >= 0 && t <= 1)
	{
		q = a + t * ab;
		return 1;
	}
	return 0;
}

std::optional<TOS_raycast_hit> TOS_ray_plane_intersect(TOS_ray ray, TOS_plane plane)
{
	std::optional<TOS_raycast_hit> hit;
	float t;
	glm::vec3 q;
	if(intersect_segment_plane(ray.origin, ray.direction * ray.t, plane, t, q))
	{
		hit = TOS_raycast_hit
		{
			.point = q,
			.normal = plane.normal
		};
	}
	return hit;
}

float closest_pt_segment_segment(glm::vec3 p1, glm::vec3 q1, glm::vec3 p2, glm::vec3 q2, float& s, float& t, glm::vec3& c1, glm::vec3& c2)
{
	glm::vec3 d1 = q1-p1;
	glm::vec3 d2 = q2-p2;
	glm::vec3 r = p1-p2;
	float a = glm::dot(d1, d1);
	float e = glm::dot(d2, d2);
	float f = glm::dot(d2, r);

	if(a <= FLT_EPSILON && e <= FLT_EPSILON)
	{
		s = t = 0.0f;
		c1 = p1;
		c2 = p2;
		return glm::dot(c1-c2, c1-c2);
	}

	if(a <= FLT_EPSILON)
	{
		s = 0.0f;
		t = f / e;
		t = glm::clamp(t, 0.0f, 1.0f);
	}
	else
	{
		float c = glm::dot(d1, r);
		if(e <= FLT_EPSILON)
		{
			t = 0.0f;
			s = glm::clamp(-c/a, 0.0f, 1.0f);
		}
		else
		{
			float b = glm::dot(d1, d2);
			float denom = a*e-b*b;

			if(denom != 0.0f)
			{
				s = glm::clamp((b*f - c*e) / denom, 0.0f, 1.0f);
			}
			else
			{
				s = 0.0f;
			}
			t = (b*s + f) / e;

			if(t < 0.0f)
			{
				t = 0.0f;
				s = glm::clamp(-c/a, 0.0f, 1.0f);
			}
			else if(t > 1.0f)
			{
				t = 1.0f;
				s = glm::clamp((b-c)/a, 0.0f, 1.0f);
			}
		}
	}

	c1 = p1 + d1 * s;
	c2 = p2 + d2 * t;
	return glm::dot(c1-c2, c1-c2);
}

float TOS_ray_segment_nearest(TOS_ray ray, TOS_segment segment, glm::vec3* ray_pt, glm::vec3* segment_pt)
{
	glm::vec3 _ray_pt;
	glm::vec3 _segment_pt;
	float s;
	float t;
	float dist = closest_pt_segment_segment
	(
		ray.origin, ray.origin + ray.direction * ray.t,
		segment.a, segment.b,
		s, t,
		_ray_pt, _segment_pt
	);
	if(ray_pt != nullptr)
		*ray_pt = _ray_pt;
	if(segment_pt != nullptr)
		*segment_pt = _segment_pt;
	return glm::sqrt(dist);
}

int intersect_ray_sphere(glm::vec3 p, glm::vec3 d, TOS_sphere s, float &t, glm::vec3& q)
{
	glm::vec3 m = p - s.center;
	float b = glm::dot(m, d);
	float c = glm::dot(m, m) - s.r * s.r;
	if(c > 0.0f && b > 0.0f)
		return 0;
	float discr = b*b - c;
	if(discr < 0.0f)
		return 0;
	t = -b - glm::sqrt(discr);
	if(t < 0.0f)
		t = 0.0f;
	q = p + t * d;
	return 1;
}

std::optional<TOS_raycast_hit> TOS_ray_sphere_intersect(TOS_ray ray, TOS_sphere sphere)
{
	std::optional<TOS_raycast_hit> hit;
	float t;
	glm::vec3 q;
	if(intersect_ray_sphere(ray.origin, ray.direction, sphere, t, q))
	{
		hit = TOS_raycast_hit
		{
			.point = q,
			.normal = glm::normalize(q - sphere.center)
		};
	}
	return hit;
}