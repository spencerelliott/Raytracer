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

#include <SDL.h>

#define FLOATCOLOR(c) int(255.99 * c)
#define DRAWPIXEL(r, g, b) out << r << " " << g << " " << b << "\n"
#define NUM_THREADS 4

struct scene_info {
	int seed;
	int w;
	int h;
	int samples;

	hitable* world;
	camera* cam;

	float* pixels;
};

vec3 color(const ray& r, hitable *world, int depth) {
	hit_record rec;
	if (world->hit(r, 0.001, FLT_MAX, rec)) {
		ray scattered;
		vec3 attenuation;

		if (depth < 50 && rec.mat_ptr->scatter(r, rec, attenuation, scattered)) {
			return attenuation * color(scattered, world, depth + 1);
		}
		else {
			return vec3(0, 0, 0);
		}
	}
	else {
		vec3 unit_direction = make_unit_vector(r.direction());
		float t = 0.5 * (unit_direction.y() + 1.0f);

		return (1.0f - t) * vec3(1.0, 1.0, 1.0) + t * vec3(0.5, 0.7, 1.0);
	}
}

HANDLE thread_mutex;

int thread_run_count = NUM_THREADS;
bool continue_threads = false;

DWORD WINAPI raytrace(LPVOID param) {
	scene_info* scene = (scene_info*)param;

	int w = scene->w;
	int h = scene->h;
	int samples = scene->samples;

	hitable* world = scene->world;
	camera cam = *scene->cam;

	while (1) {
		int pixel = 0;

		for (int j = h - 1; j >= 0; j--) {
			for (int i = 0; i < w; i++) {
				rng_state = scene->seed;

				vec3 col(0, 0, 0);

				for (int s = 0; s < samples; s++) {
					float u = float(i + random_0_to_1()) / float(w);
					float v = float(j + random_0_to_1()) / float(h);
					ray r = cam.get_ray(u, v);

					col += color(r, world, 0);
				}

				col /= float(samples);
				col = vec3(sqrt(col[0]), sqrt(col[1]), sqrt(col[2]));

				scene->pixels[pixel++] = col.r();
				scene->pixels[pixel++] = col.g();
				scene->pixels[pixel++] = col.b();

				scene->seed = rng_state;
			}
		}

		//std::cout << "Thread " << scene->seed << " finished rendering" << std::endl;

		DWORD wait_result = WaitForSingleObject(thread_mutex, INFINITE);

		thread_run_count--;

		ReleaseMutex(thread_mutex);

		while (thread_run_count < NUM_THREADS) {
			Sleep(10);
		}
	}

	return 0;
}

int main(int argc, char* argv[]) {
	vec3 lower_left(-2.0, -1.0, -1.0);
	vec3 horizontal(4.0, 0.0, 0.0);
	vec3 vertical(0.0, 2.0, 0.0);
	vec3 origin(0.0, 0.0, 0.0);

	hitable* list[4];

	list[0] = new sphere(vec3(-0.3, 0, -1), 0.5, new lambertian(vec3(0.8, 0.3, 0.3)));
	list[1] = new sphere(vec3(0, -100.5, -1), 100, new lambertian(vec3(0.8, 0.8, 0.0)));
	list[2] = new sphere(vec3(1, 0, -1.3), 0.5, new metal(vec3(0.8, 0.6, 0.2), 0.3));
	list[3] = new sphere(vec3(0.3, 0.6, -1.5), 0.4, new dielectric(2.4));

	hitable* world = new hitable_list(list, 4);

	int m = 1;
	int ms = 1;

	int w = m * 320;
	int h = m * 160;
	int samples = ms * 5;

	int samples_per_thread = samples / NUM_THREADS;

	camera cam(vec3(0, 0.4, 0.5), vec3(0, 0, -1), vec3(0, 1, 0), 90, float(w) / float(h));

	thread_mutex = CreateMutex(
		NULL,              // default security attributes
		FALSE,             // initially not owned
		NULL);

	HANDLE* threads = new HANDLE[NUM_THREADS];
	scene_info scenes[NUM_THREADS];

	long start_time = time(0);

	for (int i = 0; i < NUM_THREADS; i++) {	
		scenes[i].seed = wang_hash(i);
		scenes[i].w = w;
		scenes[i].h = h;
		scenes[i].samples = samples_per_thread;
		scenes[i].world = world;
		scenes[i].cam = &cam;
		scenes[i].pixels = new float[w * h * 3]{ 0.0 };

		threads[i] = CreateThread(NULL, 0, raytrace, &scenes[i], 0, 0);
	}

	SDL_Event event;
	SDL_Renderer* renderer;
	SDL_Window* window;

	int i;

	SDL_Init(SDL_INIT_VIDEO);

	SDL_CreateWindowAndRenderer(w, h, SDL_WINDOW_RESIZABLE, &window, &renderer);

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	SDL_RenderClear(renderer);

	SDL_Texture* texture = SDL_CreateTexture(renderer,
		SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, w, h);

	UINT32* pixels = new UINT32[w * h]{ 0 };

	while (1) {
		if (thread_run_count <= 0) {
			for (int i = 0; i < w * h * 3; i += 3) {
				float avg_r = 0.0;
				float avg_g = 0.0;
				float avg_b = 0.0;

				for (int k = 0; k < NUM_THREADS; k++) {
					float r = (float)scenes[k].pixels[i];
					float g = (float)scenes[k].pixels[i + 1];
					float b = (float)scenes[k].pixels[i + 2];

					avg_r += r;
					avg_g += g;
					avg_b += b;
				}

				int ir = FLOATCOLOR(avg_r / NUM_THREADS);
				int ig = FLOATCOLOR(avg_g / NUM_THREADS);
				int ib = FLOATCOLOR(avg_b / NUM_THREADS);

				pixels[i / 3] = (ir & 255) << 24 | (ig & 255) << 16 | (ib & 255) << 8 | 255;

			}

			SDL_UpdateTexture(texture, NULL, pixels, w * sizeof(UINT32));

			SDL_RenderClear(renderer);
			SDL_RenderCopy(renderer, texture, NULL, NULL);

			SDL_RenderPresent(renderer);

			thread_run_count = NUM_THREADS;
		}

		if (SDL_PollEvent(&event) && event.type == SDL_QUIT)
			break;
	}

	for (int i = 0; i < NUM_THREADS; i++) {
		TerminateThread(threads[i], 0);
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}