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
// The multiplier for the zoom effectiveness
#define ZOOM_FACTOR (double)(1.0 / 10.0)

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

// TODO: have the window size itself based on your monitor
// Low priority since it can be resized
static const int w = 1000;
static const int h = 800;

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
        positions = std::make_unique<double[]>(num * 3);
        velocities = std::make_unique<double[]>(num * 3);
        accelerations = std::make_unique<double[]>(num * 3);
        mus = std::make_unique<double[]>(num);
        names.reserve(num); // Vectors can be any size, so we tell it to reserve this size instead of dynamically allocating itself
        // Load positions
        for (int i = 0; i < num * 3; i++) {
            double value;
            universe.read(reinterpret_cast<char*>(&value), sizeof(value));
            positions[i] = value;
        }
        // Load velocities
        for (int i = 0; i < num * 3; i++) {
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
    SDL_Window* window = SDL_CreateWindow("Hello, world!", w, h, SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    SDL_SetRenderVSync(renderer, 1);
    

    // Draw loop variables
    double scale = 1.0 / 2.0 / (17527090.2 * 1.1);

    // Arrays used for various things inside the draw loop
    // Any array here must be overwritten before being read from
    // y is flipped from expected, but +y is completely arbitrary anyways
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
            } else if (e.type == SDL_EVENT_MOUSE_WHEEL && e.wheel.y != 0) { // The mouse wheel scrolled vertically
                scale *= 1 + e.wheel.y * ZOOM_FACTOR;
            }
        }

        // Reset the screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Determine scaling based on the window's size
        int windowWidth;
        int windowHeight;
        SDL_GetWindowSize(window, &windowWidth, &windowHeight);
        double screenScale = scale * (windowWidth < windowHeight ? windowWidth : windowHeight);
        
        // Set up the circle
        // TODO: add a `screenPositions` variable to handle all coordinate transformations
        for (size_t i = 0; i < num; i++) {
            centers[i * 2 + 0] = (float)(positions[i * 3 + 0] * screenScale + windowWidth  / 2.0);
            centers[i * 2 + 1] = (float)(positions[i * 3 + 1] * screenScale + windowHeight / 2.0);
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
        // TODO: move this elsewhere, into its own file
        
        // Perform verlet integration
        // Update acceleration
        for (size_t i = 0; i < num * 2; i++) {
            accelerations[i] = 0.0;
        }
        for (size_t i = 0; i < num; i++) {
            for (size_t j = 0; j < num; j++) {
                if (i != j) {
                    double dx = positions[j * 3 + 0] - positions[i * 3 + 0];
                    double dy = positions[j * 3 + 1] - positions[i * 3 + 1];
                    double dz = positions[j * 3 + 2] - positions[i * 3 + 2];
                    double dist2 = dx * dx + dy * dy + dz * dz;
                    double dist = sqrt(dist2);
                    double invDist3 = 1.0 / (dist2 * dist);
                    accelerations[i * 3 + 0] += dx * mus[j] * invDist3;
                    accelerations[i * 3 + 1] += dy * mus[j] * invDist3;
                    accelerations[i * 3 + 2] += dz * mus[j] * invDist3;
                }
            }
        }
        // Perform a half-step
        for (size_t i = 0; i < num * 3; i++) {
            velocities[i] += accelerations[i] * FACTOR / 2.0;
            positions[i] += velocities[i] * FACTOR;
        }
        // Update acceleration again
        for (size_t i = 0; i < num * 3; i++) {
            accelerations[i] = 0.0;
        }
        for (size_t i = 0; i < num; i++) {
            for (size_t j = 0; j < num; j++) {
                if (i != j) {
                    double dx = positions[j * 3 + 0] - positions[i * 3 + 0];
                    double dy = positions[j * 3 + 1] - positions[i * 3 + 1];
                    double dz = positions[j * 3 + 2] - positions[i * 3 + 2];
                    double dist2 = dx * dx + dy * dy + dz * dz;
                    double dist = sqrt(dist2);
                    double invDist3 = 1.0 / (dist2 * dist);
                    accelerations[i * 3 + 0] += dx * mus[j] * invDist3;
                    accelerations[i * 3 + 1] += dy * mus[j] * invDist3;
                    accelerations[i * 3 + 2] += dz * mus[j] * invDist3;
                }
            }
        }
        // Another half-step
        for (size_t i = 0; i < num * 3; i++) {
            velocities[i] += accelerations[i] * FACTOR / 2.0;
        }
    }
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}