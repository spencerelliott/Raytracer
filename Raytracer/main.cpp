#include <iostream>
#include <fstream>
#include <vec3.h>
#include <ray.h>

#define FLOATCOLOR(c) int(255.99 * c)
#define DRAWPIXEL(r, g, b) std::cout << r << " " << g << " " << b << "\n"

vec3 color(const ray& r) {
	vec3 unit_direction = make_unit_vector(r.direction());
	float t = 0.5 * (unit_direction.y() + 1.0f);

	return (1.0f - t) * vec3(1.0, 1.0, 1.0) + t * vec3(0.5, 0.7, 1.0);
}

int main() {
	std::ofstream out("out.ppm");
	std::cout.rdbuf(out.rdbuf());

	vec3 lower_left(-2.0, -1.0, -1.0);
	vec3 horizontal(4.0, 0.0, 0.0);
	vec3 vertical(0.0, 2.0, 0.0);
	vec3 origin(0.0, 0.0, 0.0);

	int w = 200;
	int h = 100;
	std::cout << "P3\n" << w << " " << h << "\n255\n";
	for (int j = h - 1; j >= 0; j--) {
		for (int i = 0; i < w; i++) {
			float u = float(i) / float(w);
			float v = float(j) / float(h);

			vec3 col = color(ray(origin, lower_left + u*horizontal + v*vertical));

			int ir = FLOATCOLOR(col.r());
			int ig = FLOATCOLOR(col.g());
			int ib = FLOATCOLOR(col.b());

			DRAWPIXEL(ir, ig, ib);
		}
	}
}