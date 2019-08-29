#include <iostream>
#include <fstream>
#include <float.h>

#include "vec3.h"
#include "ray.h"
#include "sphere.h"
#include "hitable_list.h"
#include "camera.h"

#define FLOATCOLOR(c) int(255.99 * c)
#define DRAWPIXEL(r, g, b) std::cout << r << " " << g << " " << b << "\n"

vec3 color(const ray& r, hitable *world) {
	hit_record rec;
	if (world->hit(r, 0.0, FLT_MAX, rec)) {
		return 0.5 * vec3(rec.normal.x() + 1, rec.normal.y() + 1, rec.normal.z() + 1);
	}
	else {
		vec3 unit_direction = make_unit_vector(r.direction());
		float t = 0.5 * (unit_direction.y() + 1.0f);

		return (1.0f - t) * vec3(1.0, 1.0, 1.0) + t * vec3(0.5, 0.7, 1.0);
	}
}

int main() {
	std::ofstream out("out.ppm");
	std::cout.rdbuf(out.rdbuf());

	vec3 lower_left(-2.0, -1.0, -1.0);
	vec3 horizontal(4.0, 0.0, 0.0);
	vec3 vertical(0.0, 2.0, 0.0);
	vec3 origin(0.0, 0.0, 0.0);

	hitable* list[2];

	list[0] = new sphere(vec3(0, 0, -1), 0.5);
	list[1] = new sphere(vec3(0, -100.5, -1), 100);

	hitable* world = new hitable_list(list, 2);

	camera cam;

	int w = 200;
	int h = 100;
	int samples = 100;
	std::cout << "P3\n" << w << " " << h << "\n255\n";
	for (int j = h - 1; j >= 0; j--) {
		for (int i = 0; i < w; i++) {
			vec3 col(0, 0, 0);

			for (int s = 0; s < samples; s++) {
				float u = float(i + ((float)rand() / (RAND_MAX))) / float(w);
				float v = float(j + ((float)rand() / (RAND_MAX))) / float(h);
				ray r = cam.get_ray(u, v);

				col += color(r, world);
			}

			col /= float(samples);

			int ir = FLOATCOLOR(col.r());
			int ig = FLOATCOLOR(col.g());
			int ib = FLOATCOLOR(col.b());

			DRAWPIXEL(ir, ig, ib);
		}
	}
}