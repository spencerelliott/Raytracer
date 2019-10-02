#include <array>
#include <iostream>
#include <fstream>
#include <float.h>
#include <Windows.h>
#include <time.h>

#include "vec3.h"
#include "ray.h"
#include "sphere.h"
#include "hitable_list.h"
#include "camera.h"
#include "random.h"
#include "material.h"

#include "SDL.h"

#define FLOATCOLOR(c) int(255.99 * c)

#define NUM_THREADS 4

#define BASE_WIDTH 320
#define BASE_HEIGHT 240

#define R(r) (r >> 24)
#define G(g) (g >> 16 & 0xFF)
#define B(b) (b >> 8 & 0xFF)

#define RGB(r, g, b) (r & 255) << 24 | (g & 255) << 16 | (b & 255) << 8 | 255

struct scene_info {
	int seed;
	int w;
	int h;
	int samples;

	hitable* world;
	camera* cam;

	vec3* pixels;
};

void box_blur(UINT32* pixels, UINT32* dest, int w, int h) {
	for (int i = 0; i < w * h; i++) {
		int x = i % w;
		int y = i / w;

		if (x > 0 && x < w - 1 && y > 0 && y < h - 1) {
			int upper_row = R(pixels[i - w - 1]) + R(pixels[i - w]) + R(pixels[i - w + 1]);
			int mid_row = R(pixels[i - 1]) + R(pixels[i]) + R(pixels[i + 1]);
			int lower_row = R(pixels[i + w - 1]) + R(pixels[i + w]) + R(pixels[i + w + 1]);

			int r_blur = (upper_row + mid_row + lower_row) / 9; 

			upper_row = G(pixels[i - w - 1]) + G(pixels[i - w]) + G(pixels[i - w + 1]);
			mid_row = G(pixels[i - 1]) + G(pixels[i]) + G(pixels[i + 1]);
			lower_row = G(pixels[i + w - 1]) + G(pixels[i + w]) + G(pixels[i + w + 1]);

			int g_blur = (upper_row + mid_row + lower_row) / 9;

			upper_row = B(pixels[i - w - 1]) + B(pixels[i - w]) + B(pixels[i - w + 1]);
			mid_row = B(pixels[i - 1]) + B(pixels[i]) + B(pixels[i + 1]);
			lower_row = B(pixels[i + w - 1]) + B(pixels[i + w]) + B(pixels[i + w + 1]);

			int b_blur = (upper_row + mid_row + lower_row) / 9;

			dest[i] = RGB(r_blur, g_blur, b_blur);
		}
		else {
			dest[i] = pixels[i];
		}
	}
}

vec3 color(const ray& r, hitable* world, int depth) {
	hit_record rec;
	if (world->hit(r, 0.001f, FLT_MAX, rec)) {
		ray scattered;
		vec3 attenuation;

		if (depth < 100 && rec.mat_ptr->scatter(r, rec, attenuation, scattered)) {
			return attenuation * color(scattered, world, depth + 1);
		}
		else {
			return vec3(0, 0, 0);
		}
	}
	else {
		vec3 unit_direction = make_unit_vector(r.direction());
		float t = 0.5f * (unit_direction.y() + 1.0f);

		return (1.0f - t) * vec3(1.0f, 1.0f, 1.0f) + t * vec3(0.5f, 0.7f, 1.0f);
	}
}

HANDLE thread_mutex;

int thread_run_count = NUM_THREADS;

void decrement_thread_count() {
	DWORD wait_result = WaitForSingleObject(thread_mutex, INFINITE);

	thread_run_count--;

	ReleaseMutex(thread_mutex);
}

DWORD WINAPI raytrace(LPVOID param) {
	scene_info* scene = (scene_info*)param;

	int w = scene->w;
	int h = scene->h;
	int samples = scene->samples;

	hitable* world = scene->world;

	while (1) {
		camera cam = *scene->cam;
		int pixel = 0;

		for (int j = h - 1; j >= 0; j--) {
			for (int i = 0; i < w; i++) {
				rng_state = scene->seed;

				vec3 col(0.0f, 0.0f, 0.0f);

				for (int s = 0; s < samples; s++) {
					float u = float(i + random_0_to_1()) / float(w);
					float v = float(j + random_0_to_1()) / float(h);
					ray r = cam.get_ray(u, v);

					col += color(r, world, 0);
				}

				col /= float(samples);
				col = vec3(sqrt(col[0]), sqrt(col[1]), sqrt(col[2]));

				scene->pixels[pixel++] = col;
				scene->seed = rng_state;
			}
		}

		decrement_thread_count();

		while (thread_run_count < NUM_THREADS) {
			Sleep(10);
		}
	}

	return 0;
}

int main(int argc, char* argv[]) {
	std::array<hitable*, 4> list{ {
			new sphere(vec3(-0.3f, 0.0f, -1.0f), 0.5f, new lambertian(vec3(0.8f, 0.3f, 0.3f))),
			new sphere(vec3(0.0f, -100.5f, -1.0f), 100, new lambertian(vec3(0.8f, 0.8f, 0.0f))),
			new sphere(vec3(1.0f, 0.0f, -1.3f), 0.5f, new metal(vec3(0.8f, 0.6f, 0.2f), 0.3f)),
			new sphere(vec3(0.3f, 0.6f, -1.5f), 0.4f, new dielectric(2.4f)) } };

	hitable* world = new hitable_list<4>(list);

	int resolution_multiplier = 2;
	int sample_multiplier = 1;

	int w = resolution_multiplier * BASE_WIDTH;
	int h = resolution_multiplier * BASE_HEIGHT;
	int samples = sample_multiplier * 5;

	int samples_per_thread = samples / NUM_THREADS;

	camera cam(vec3(0.0f, 0.4f, 0.5f), vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, 1.0f, 0.0f), 90, float(w) / float(h));

	thread_mutex = CreateMutex(NULL, FALSE, NULL);

	HANDLE* threads = new HANDLE[NUM_THREADS];
	scene_info scenes[NUM_THREADS];

	for (int i = 0; i < NUM_THREADS; i++) {
		scenes[i].seed = wang_hash(i);
		scenes[i].w = w;
		scenes[i].h = h;
		scenes[i].samples = 1;
		scenes[i].world = world;
		scenes[i].cam = &cam;
		scenes[i].pixels = new vec3[w * h]{ vec3(0.0f, 0.0f, 0.0f) };

		threads[i] = CreateThread(NULL, 0, raytrace, &scenes[i], 0, 0);
	}

	SDL_Event event;
	SDL_Renderer* renderer;
	SDL_Window* window;

	SDL_Init(SDL_INIT_VIDEO);

	SDL_CreateWindowAndRenderer(w, h, SDL_WINDOW_RESIZABLE, &window, &renderer);

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	SDL_RenderClear(renderer);

	SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, w, h);

	UINT32* buffer = new UINT32[w * h]{ 0 };
	UINT32* pixels = new UINT32[w * h]{ 0 };
	unsigned int row_size = w * sizeof(UINT32);

	int current_spin_count = 0;

	while (1) {
		if (thread_run_count <= 0) {
			for (int i = 0; i < w * h; i++) {
				float avg_r = 0;
				float avg_g = 0;
				float avg_b = 0;

				for (int k = 0; k < NUM_THREADS; k++) {
					vec3 pixel = scenes[k].pixels[i];

					avg_r += pixel.r();
					avg_g += pixel.g();
					avg_b += pixel.b();
				}

				int ir = FLOATCOLOR(avg_r / NUM_THREADS);
				int ig = FLOATCOLOR(avg_g / NUM_THREADS);
				int ib = FLOATCOLOR(avg_b / NUM_THREADS);

				buffer[i] = (ir & 255) << 24 | (ig & 255) << 16 | (ib & 255) << 8 | 255;

			}

			box_blur(buffer, pixels, w, h);

			SDL_UpdateTexture(texture, NULL, pixels, row_size);

			SDL_RenderClear(renderer);
			SDL_RenderCopy(renderer, texture, NULL, NULL);

			SDL_RenderPresent(renderer);

			thread_run_count = NUM_THREADS;

			cam.set_look(vec3(2.0f * sin(float(current_spin_count) * M_PI / 180), 0.4f, 0.5f), vec3(0.0f, 0.0f, -1.0f));
			current_spin_count = ++current_spin_count % 360;
		}

		if (SDL_PollEvent(&event) && event.type == SDL_QUIT) {
			break;
		}
	}

	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	for (int i = 0; i < NUM_THREADS; i++) {
		TerminateThread(threads[i], 0);
		CloseHandle(threads[i]);
		delete[] scenes[i].pixels;
	}

	delete[] threads;
	delete[] pixels;

	delete world;

	for (unsigned int i = 0; i < list.size(); i++) {
		delete list[i];
	}

	return 0;
}