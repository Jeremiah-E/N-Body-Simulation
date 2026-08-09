#include <windows.h>
#include <SDL3/SDL.h>
#include <memory>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <cstdio>

// The number of triangles in each circle
#define resolution 8
// The multiplier for the simulation's speed. Will be redone later
#define FACTOR 60

// Precomputed sine and cosine values for generating circles
static double sines[resolution];
static double cosines[resolution];
static bool sincosInit = false;

// Given an array of centers, store the points of the circle in the vertex array
void createCircles(size_t num, float *centers, SDL_Vertex *verts, int *idxs, double size) {
    // Precompute angles. Ideally, this'd be done in WinMain as I want to try and make this multithreaded later
    // For now, I'll leave it since I've already done way more premature optimization than I should've
    if (!sincosInit) {
        for (size_t i = 0; i < resolution; i++) {
            double angle = ((double)i / resolution) * SDL_PI_D * 2.0;
            sines[i] = sin(angle);
            cosines[i] = cos(angle);
        }
        sincosInit = true;
    }

    // We assume verts is [resolution+1] times the length of num
    // With that, we go center by center and mutate verts
    // Since each iteration does not mess with the last, I hint the compiler parallelize it
    #pragma loop(hint_parallel(0))
    for (size_t i = 0; i < num; i++) {
        double cx = centers[i * 2 + 0];
        double cy = centers[i * 2 + 1];
        // Our local verts array
        SDL_Vertex *_verts = verts + (resolution + 1) * i;
        for (size_t j = 0; j < resolution; j++) {
            // The jth point along a circle
            _verts[j].position = {(float)(cx + sines[j] * size), (float)(cy + cosines[j] * size)};
        }
        // The center of the circle, all tris forming the circle point here
        _verts[resolution].position = {(float)cx, (float)cy};
    }

    // Clear the color to red
    const size_t numVert = num * (resolution + 1);
    for (size_t i = 0; i < numVert; i++) {
        verts[i].color = {1, 0, 0, 1};
    }

    // Build triangle indices for each circle
    for (size_t i = 0; i < num; i++) {
        int base = (resolution + 1) * i;
        int ibase = resolution * 3 * i;
        for (int j = 0; j < resolution; j++) {
            int tri = ibase + j * 3;
            idxs[tri + 0] = base + j;
            idxs[tri + 1] = base + ((j + 1) % resolution);
            idxs[tri + 2] = base + resolution;
        }
    }
}

static const int w = 800;
static const int h = 600;

// A "hello, world" program of sorts
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    // Initialize data
    int num;
    std::unique_ptr<double []> positions;
    std::unique_ptr<double []> velocities;
    std::unique_ptr<double []> accelerations;
    std::unique_ptr<double []> mus;
    std::vector<std::string> names;
    // Read from universe.bin
    std::ifstream universe("universe.bin", std::ios::binary);
    if (universe.is_open()) {
        // Get num
        universe.read(reinterpret_cast<char*>(&num), sizeof(int));
        // Create the pointer arrays/vector for everything
        positions = std::make_unique<double[]>(num * 2);
        velocities = std::make_unique<double[]>(num * 2);
        accelerations = std::make_unique<double[]>(num * 2);
        mus = std::make_unique<double[]>(num);
        names.reserve(num); // Vectors can be any size, so we tell it to reserve this size instead of dynamically allocating itself
        // Load positions
        for (int i = 0; i < num * 2; i++) {
            double value;
            universe.read(reinterpret_cast<char*>(&value), sizeof(value));
            positions[i] = value;
        }
        // Load velocities
        for (int i = 0; i < num * 2; i++) {
            double value;
            universe.read(reinterpret_cast<char*>(&value), sizeof(value));
            velocities[i] = value;
        }
        // Load mus
        for (int i = 0; i < num; i++) {
            double value;
            universe.read(reinterpret_cast<char*>(&value), sizeof(value));
            mus[i] = value;
        }
        // Load names
        for (int i = 0; i < num; i++) {
            std::string name;
            // Get up to the null-terminator
            std::getline(universe, name, '\0');
            // Add it to names
            names.push_back(std::move(name));
        }
        
        universe.close();
    } else {
        // The file could not open
        return 1;
    }

    // Create the window
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Hello, world!", w, h, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    SDL_SetRenderVSync(renderer, 1);

    // Draw loop variables. For now a const, when I get futher along I'll mess with this a bit
    const double scale = min(w, h) / 2.0 / (17527090.2 * 1.1);

    // Arrays used for various things inside the draw loop
    // Any array here must be overwritten before being read from
    auto centers = std::make_unique<float[]>(num*2);
    auto verts = std::make_unique<SDL_Vertex[]>((resolution + 1) * num);
    auto idxs = std::make_unique<int[]>(num*resolution*3);

    bool running = true;
    while (running) {
        // Gets the current events
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) { // The user/computer asked the program to quit
                // Terminate the loop
                running = false;
            }
        }

        // Reset the screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        // Set up the circle
        // When I allow for rotation, I will make this its own function
        for (size_t i = 0; i < num; i++) {
            centers[i*2] = positions[i * 2 + 0] * scale + w / 2;
            centers[i*2+1] = -positions[i * 2 + 1] * scale + h / 2;
        }
        createCircles(num, centers.get(), verts.get(), idxs.get(), 10);
        
        // Draw the circles
        SDL_RenderGeometry(renderer, NULL, verts.get(), (resolution + 1) * num, idxs.get(), num * resolution * 3);

        // Draw some text
        const size_t bufLen = 50;
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        char buf[bufLen];
        // Row 1: "Simulating 2 bodies"
        snprintf(buf, bufLen, "Simulating %d bodies", num);
        SDL_RenderDebugText(renderer, 10, 10, buf);
        // Row 2: "Pluto, Charon"
        size_t bufIdx = 0;
        for (size_t i = 0; i + 1 < static_cast<size_t>(num); i++) {
            snprintf(buf + bufIdx, bufLen - bufIdx, "%s, ", names[i].c_str());
            bufIdx += names[i].size() + 2;
        }
        if (num > 0) {
            snprintf(buf + bufIdx, bufLen - bufIdx, "%s", names[num - 1].c_str());
        }
        SDL_RenderDebugText(renderer, 10, 20, buf);

        SDL_RenderPresent(renderer);

        // We're out of our draw loop and into our computation loop
        
        // Perform verlet integration
        // Update acceleration
        for (size_t i = 0; i < num * 2; i++) {
            accelerations[i] = 0.0;
        }
        for (size_t i = 0; i < num; i++) {
            for (size_t j = 0; j < num; j++) {
                if (i != j) {
                    double dx = positions[j * 2 + 0] - positions[i * 2 + 0];
                    double dy = positions[j * 2 + 1] - positions[i * 2 + 1];
                    double dist2 = dx * dx + dy * dy;
                    double dist = sqrt(dist2);
                    double invDist3 = 1.0 / (dist2 * dist);
                    accelerations[i * 2 + 0] += dx * mus[j] * invDist3;
                    accelerations[i * 2 + 1] += dy * mus[j] * invDist3;
                }
            }
        }
        // Perform a half-step
        for (size_t i = 0; i < num * 2; i++) {
            velocities[i] += accelerations[i] * FACTOR / 2.0;
            positions[i] += velocities[i] * FACTOR;
        }
        // Update acceleration again
        for (size_t i = 0; i < num * 2; i++) {
            accelerations[i] = 0.0;
        }
        for (size_t i = 0; i < num; i++) {
            for (size_t j = 0; j < num; j++) {
                if (i != j) {
                    double dx = positions[j * 2 + 0] - positions[i * 2 + 0];
                    double dy = positions[j * 2 + 1] - positions[i * 2 + 1];
                    double dist2 = dx * dx + dy * dy;
                    double dist = sqrt(dist2);
                    double invDist3 = 1.0 / (dist2 * dist);
                    accelerations[i * 2 + 0] += dx * mus[j] * invDist3;
                    accelerations[i * 2 + 1] += dy * mus[j] * invDist3;
                }
            }
        }
        // Another half-step
        for (size_t i = 0; i < num * 2; i++) {
            velocities[i] += accelerations[i] * FACTOR / 2.0;
        }
    }
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}