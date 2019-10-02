#pragma once

#include <array>
#include "hitable.h"

template<typename std::size_t S>
class hitable_list : public hitable {
public:
	hitable_list() { }
	hitable_list(std::array<hitable*, S> l) { list = l;}

	virtual bool hit(const ray& r, float t_min, float t_max, hit_record& rec) const;

	std::array<hitable*, S> list;
	int list_size;
};

template<typename std::size_t S>
bool hitable_list<S>::hit(const ray& r, float t_min, float t_max, hit_record& rec) const {
	bool hit_anything = false;

	hit_record temp_rec;

	float closest = t_max;

	for (int i = 0; i < S; i++) {
		if (list[i]->hit(r, t_min, closest, temp_rec)) {
			hit_anything = true;
			closest = temp_rec.t;
			rec = temp_rec;
		}
	}

	return hit_anything;
}