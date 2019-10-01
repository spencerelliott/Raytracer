#pragma once

#include "ray.h"

class camera {
public:
	camera(vec3 look_from, vec3 look_at, vec3 up, float vfov, float aspect) : up(up), fov(vfov), aspect(aspect) {
		set_look(look_from, look_at);
	}

	void set_look(vec3 look_from, vec3 look_at) {
		vec3 u, v, w;

		float theta = fov * M_PI / 180;
		float half_height = tan(theta / 2);
		float half_width = aspect * half_height;

		origin = look_from;
		w = make_unit_vector(look_from - look_at);
		u = make_unit_vector(cross(up, w));
		v = cross(w, u);

		lower_left = vec3(-half_width, -half_height, -1);
		lower_left = origin - half_width * u - half_height * v - w;

		horizontal = 2 * half_width * u;
		vertical = 2 * half_height * v;
	}

	ray get_ray(float u, float v) { return ray(origin, lower_left + u * horizontal + v * vertical - origin); }

	vec3 origin;
	vec3 lower_left;
	vec3 horizontal;
	vec3 vertical;

	vec3 up;

	float fov;
	float aspect;
};