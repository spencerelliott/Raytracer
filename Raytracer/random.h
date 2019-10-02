#pragma once

#include <cstdlib>
#include "vec3.h"

UINT rng_state;

inline UINT wang_hash(int seed) {
	seed = (seed ^ 61) ^ (seed >> 16);
	seed *= 9;
	seed = seed ^ (seed >> 4);
	seed *= 0x27d4eb2d;
	seed = seed ^ (seed >> 15);

	return seed;
}

inline UINT random_uint() {
	rng_state = 1664525 * rng_state + 1013904223;

	return rng_state;
}

inline UINT random_xorshift() {
	rng_state ^= (rng_state << 13);
	rng_state ^= (rng_state >> 17);
	rng_state ^= (rng_state << 5);

	return rng_state;
}

inline float random_0_to_1() {
	return float(random_xorshift()) / float(UINT_MAX);
}

inline float random_float() {
	return (2.0f * random_0_to_1()) - 1.0f;
}

vec3 random_in_unit_sphere() {
	vec3 p;
	do {
		p = 2.0f * vec3(random_float(), random_float(), random_float()) - vec3(1, 1, 1);
	} while (p.squared_length() >= 1.0);
	return p;
}