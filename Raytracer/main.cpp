#include <iostream>
#include <fstream>

#define FLOATCOLOR(c) int(255.99 * c)
#define DRAWPIXEL(r, g, b) std::cout << r << " " << g << " " << b << "\n"

int main() {
	std::ofstream out("out.ppm");
	std::cout.rdbuf(out.rdbuf());

	int w = 200;
	int h = 100;
	std::cout << "P3\n" << w << " " << h << "\n255\n";
	for (int j = h - 1; j >= 0; j--) {
		for (int i = 0; i < w; i++) {
			float r = float(i) / float(w);
			float g = float(j) / float(h);
			float b = 0.2f;

			int ir = FLOATCOLOR(r);
			int ig = FLOATCOLOR(g);
			int ib = FLOATCOLOR(b);

			DRAWPIXEL(ir, ig, ib);
		}
	}
}