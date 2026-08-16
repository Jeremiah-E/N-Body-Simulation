#include <windows.h>
#include <SDL3/SDL.h>
#include <memory>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <cstdio>
#include <type_traits>
#include <algorithm>
#include "vector.hpp" // Vec3D struct

#pragma region Compile-Time Vars

// The number of triangles in each circle
#define RESOLUTION 20
// The multiplier for the simulation's speed. Will be redone later
#define FACTOR 10
// The multiplier for the zoom effectiveness
#define ZOOM_FACTOR (double)(1.0 / 10.0)
// To flip the y axis. Completely arbitrary, might align to the ecliptic, depending on how the data's structured
static Vec3D<double> GLOBAL_UP = Vec3D<double>(0.0, -1.0, 0.0).norm();

// Precomputed sine and cosine values for generating circles
static double sines[RESOLUTION];
static double cosines[RESOLUTION];
static bool sincosInit = false;

// Camera constants
static double const fov = 1.5707963; // 90°
static double const projectionScale = 1.0 / tan(fov / 2.0); // Exactly 1, but we keep it here incase we want to change fov
// Camera variables
static double camDist = 5.979e10; // The distance from the cIdxth body
static size_t cIdx = 0; // The camera is camDist away from {positions[cIdx*3+0], positions[cIdx*3+1], positions[cIdx*3+2]}
static float minScale = 3; // The minimum size of a body. When textures are introduced, this will determine when something's a point
// Camera directions
static Vec3D<double> forwards = {0, 0, 1};
static Vec3D<double> up = {0, 1, 0};
static Vec3D<double> right = {1, 0, 0};

#pragma endregion Compile-Time Vars

#pragma region Circle Drawing
// Given an array of centers, store the points of the circle in the vertex array
void createCircles(const size_t num, std::vector<Vec3D<float>> centers, SDL_Vertex *verts, int *idxs, float *sizes, double *newNum, const size_t cIdx) {
    // Precompute angles. Ideally, this'd be done in WinMain as I want to try and make this multithreaded later
    // For now, I'll leave it since I've already done way more premature optimization than I should've
    if (!sincosInit) {
        for (size_t i = 0; i < RESOLUTION; i++) {
            double angle = ((double)i / RESOLUTION) * SDL_PI_D * 2.0;
            sines[i] = sin(angle);
            cosines[i] = cos(angle);
        }
        sincosInit = true;
    }

    // Filter out entries with invalid size and compact the arrays
    // This culls shapes off screen, given how this function is used in the draw loop
    size_t validIdx = 0;
    size_t postCullCIdx = cIdx;
    bool isCIdxCulled = false;
    for (size_t i = 0; i < num; i++) {
        if (sizes[i] <= 0.0) {
            // i was invalid, so we know there's one less valid object
            (*newNum)--;
            // If we get here, sizes[cIdx] was invalid
            if (i == cIdx) { isCIdxCulled = true; }
        } else {
            // Move cIdx as needed
            if (i == cIdx) {
                postCullCIdx = validIdx;
            }
            // Move this entry to validIdx position
            if (validIdx != i) {
                centers[validIdx] = centers[i];
                sizes[validIdx] = sizes[i];
            }
            validIdx++;
        }
    }

    // Sort entries by their z value, for draw-order reasons
    // We create a list of indices, for now in original order
    std::vector<size_t> sortedIndices(*newNum);
    for (size_t i = 0; i < *newNum; i++) {
        sortedIndices[i] = i;
    }
    // Sort the values by the z coordinate of centers
    std::sort(
        // Bounds
        sortedIndices.begin(), sortedIndices.end(),
        // Lambda function
        [&centers](size_t a, size_t b) {return centers[a].z > centers[b].z;}
    );
    // Store everything in temp arrays according to sortedIndices
    std::vector<Vec3D<float>> tempCenters(*newNum);
    std::vector<float> tempSizes(*newNum);
    for (size_t i = 0; i < *newNum; i++) {
        tempCenters[i] = centers[sortedIndices[i]];
        tempSizes[i] = sizes[sortedIndices[i]];
    }
    // Insert the temp arrays back
    for (size_t i = 0; i < *newNum; i++) {
        centers[i] = tempCenters[i];
        sizes[i] = tempSizes[i];
    }
    // Store the old indices
    std::vector<size_t> inverseSortedIndices(*newNum);
    for (size_t i = 0; i < *newNum; i++) {
        inverseSortedIndices[sortedIndices[i]] = i;
    }

    // We assume verts is [resolution+1] times the length of newNum
    // With that, we go center by center and mutate verts
    // Since each iteration does not mess with the last, I hint the compiler parallelize it
    #pragma loop(hint_parallel(0))
    for (size_t i = 0; i < (size_t)*newNum; i++) {
        const double cx = centers[i].x;
        const double cy = centers[i].y;
        // Our local verts array
        SDL_Vertex *_verts = verts + (RESOLUTION + 1) * i;
        for (size_t j = 0; j < RESOLUTION; j++) {
            // The jth point along a circle
            _verts[j].position = {(float)(cx + sines[j] * sizes[i]), (float)(cy + cosines[j] * sizes[i])};
        }
        // The center of the circle, all tris forming the circle point here
        _verts[RESOLUTION].position = {(float)cx, (float)cy};
    }

    // Clear the color to red or blue, depending on cIdx
    const size_t numVert = (size_t)*newNum * (RESOLUTION + 1);
    size_t finalCIdx = isCIdxCulled ? (size_t)-1 : inverseSortedIndices[postCullCIdx];
    for (size_t i = 0; i < numVert; i++) {
        if (!isCIdxCulled && (i / (RESOLUTION + 1) == finalCIdx)) {
            verts[i].color = {0, 0, 1, 1}; // Blue
        } else {
            verts[i].color = {1, 0, 0, 1}; // Red
        }
    }

    // Build triangle indices for each circle
    for (size_t i = 0; i < (size_t)*newNum; i++) {
        int base = (RESOLUTION + 1) * i;
        int ibase = RESOLUTION * 3 * i;
        for (int j = 0; j < RESOLUTION; j++) {
            int tri = ibase + j * 3;
            idxs[tri + 0] = base + j;
            idxs[tri + 1] = base + ((j + 1) % RESOLUTION);
            idxs[tri + 2] = base + RESOLUTION;
        }
    }
}
#pragma endregion Circle Drawing

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    #pragma region File Loading
    // Initialize data
    int num;
    std::vector<Vec3D<double>> positions;
    std::vector<Vec3D<double>> velocities;
    std::vector<Vec3D<double>> accelerations;
    std::vector<double> mus;
    std::vector<double> radii;
    std::vector<std::string> names;
    // Read from universe.bin
    std::ifstream universe("universe.bin", std::ios::binary);
    if (universe.is_open()) {
        // Get num
        universe.read(reinterpret_cast<char*>(&num), sizeof(int));
        // Create the pointer arrays/vector for everything
        positions.resize(num); velocities.resize(num); accelerations.resize(num);
        mus.resize(num);       names.reserve(num);     radii.resize(num);
        // Load positions
        for (int i = 0; i < num; i++) {
            positions[i] = Vec3D<double>();
            universe.read(reinterpret_cast<char*>(&(positions[i].x)), sizeof(double));
            universe.read(reinterpret_cast<char*>(&(positions[i].y)), sizeof(double));
            universe.read(reinterpret_cast<char*>(&(positions[i].z)), sizeof(double));
        }
        // Load velocities
        for (int i = 0; i < num; i++) {
            velocities[i] = Vec3D<double>();
            universe.read(reinterpret_cast<char*>(&(velocities[i].x)), sizeof(double));
            universe.read(reinterpret_cast<char*>(&(velocities[i].y)), sizeof(double));
            universe.read(reinterpret_cast<char*>(&(velocities[i].z)), sizeof(double));
        }
        // Load mus
        for (int i = 0; i < num; i++) {
            universe.read(reinterpret_cast<char*>(&mus[i]), sizeof(double));
        }
        // Load names
        for (int i = 0; i < num; i++) {
            std::string name;
            // Get up to the null-terminator
            std::getline(universe, name, '\0');
            // Add it to names
            names.push_back(std::move(name));
        }
        // Load radii
        for (int i = 0; i < num; i++) {
            universe.read(reinterpret_cast<char*>(&radii[i]), sizeof(double));
        }
        
        universe.close();
    } else {
        // The file could not open
        return 1;
    }
    #pragma endregion File Loading

    #pragma region Create Window
    SDL_Init(SDL_INIT_VIDEO);
    // Get monitor details
    int count;
    SDL_DisplayID* displays = SDL_GetDisplays(&count);
    const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(displays[0]);
    SDL_free(displays);
    // Now create the window
    SDL_Window* window = SDL_CreateWindow("Solar System", mode->w / 2.0, mode->h / 2.0, SDL_WINDOW_RESIZABLE);
    // Move it to the desired location, centering it on the screen
    SDL_SetWindowPosition(window, mode->w / 4.0, mode->h / 4.0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    SDL_SetRenderVSync(renderer, 1);
    #pragma endregion Create Window
    
    // Arrays used for various things inside the draw loop
    // Any array here must be overwritten before being read from
    auto sizes = std::make_unique<float[]>(num);
    std::vector<Vec3D<float>> centers(num);
    auto verts = std::make_unique<SDL_Vertex[]>((RESOLUTION + 1) * num);
    auto idxs = std::make_unique<int[]>(num*RESOLUTION*3);
    // Initialize centers
    for (size_t i = 0; i < num; i++) {
        centers[i] = Vec3D<float>();
    }

    bool running = true;
    while (running) {
        #pragma region Event Loop
        // Gets the current events
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) { // The user/computer asked the program to quit
                // Terminate the loop
                running = false;
            }
            if (e.type == SDL_EVENT_MOUSE_WHEEL && e.wheel.y != 0) { // The mouse wheel scrolled vertically
                camDist *= 1 - e.wheel.y * ZOOM_FACTOR;
            } 
            if (e.type == SDL_EVENT_MOUSE_MOTION && (e.motion.state & SDL_BUTTON_LMASK)) {
                // Get how much to rotate everything by
                double yaw = e.motion.xrel * 0.008;
                double pitch = e.motion.yrel * 0.005;
                // Rotate everything
                forwards = forwards.rotate(GLOBAL_UP, yaw);
                right = right.rotate(GLOBAL_UP, yaw);
                forwards = forwards.rotate(right, pitch);
                // Normalize and recalculate everything
                forwards = forwards.norm(); // Renormalize forwards (floating point drift)
                right = (right - forwards * (right * forwards)).norm(); // Reorthogonalize right
                up = (right ^ forwards).norm(); // Recompute up
            }
            if (e.type == SDL_EVENT_KEY_DOWN) {
                if (e.key.key == SDLK_LEFTBRACKET) {
                    cIdx = (cIdx - 1 + num) % num;
                }
                if (e.key.key == SDLK_RIGHTBRACKET) {
                    cIdx = (cIdx + 1) % num;
                }
            }
        }
        #pragma endregion Event Loop

        #pragma region Draw Loop
        // Reset the screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        #pragma region Camera
        // Determine scaling based on the window's size
        int windowWidth;
        int windowHeight;
        SDL_GetWindowSize(window, &windowWidth, &windowHeight);
        
        // The camera is pointing at positions[cIdx] at distance camDist
        Vec3D<double> cameraPos = positions[cIdx] - camDist * forwards;
        #pragma endregion Camera

        #pragma region Projection
        // Set up the circle
        for (size_t i = 0; i < num; i++) {
            // The new, perspective method
            const Vec3D<double> worldPos = positions[i] - cameraPos;

            const double xView = worldPos * right;
            const double yView = worldPos * up;
            const double zView = worldPos * forwards;

            if (zView > 0) {
                double projFactor = (windowHeight / 2.0) * projectionScale / zView;
                centers[i].x = xView * projFactor + windowWidth / 2.0;
                centers[i].y = yView * projFactor + windowHeight / 2.0;
                centers[i].z = zView; // Used for culling/draw order
                sizes[i] = max((float)(radii[i] * projFactor), minScale);
            } else {
                sizes[i] = -1;
            }
        }
        double newNum = num;
        createCircles(num, centers, verts.get(), idxs.get(), sizes.get(), &newNum, cIdx);

        // Draw all circles in a single batched call
        SDL_RenderGeometry(renderer, NULL, verts.get(), newNum * (RESOLUTION + 1), idxs.get(), newNum * RESOLUTION * 3);
        #pragma endregion Projection

        #pragma region Text
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        // Row 1: "Simulating 2 bodies"
        std::string buf = std::string("Simulating ") + std::to_string(num) + std::string(" bodies");
        SDL_RenderDebugText(renderer, 10, 10, buf.c_str());
        // Row 2: "Centered: Pluto"
        buf = std::string("Centered: ") + names[cIdx];
        SDL_RenderDebugText(renderer, 10, 20, buf.c_str());
        #pragma endregion Text

        SDL_RenderPresent(renderer);
        #pragma endregion Draw Loop

        #pragma region Computation Loop
        // Perform verlet integration
        // Update acceleration
        for (size_t i = 0; i < num; i++) {
            accelerations[i] = Vec3D<double>();
        }
        for (size_t i = 0; i < num; i++) {
            for (size_t j = 0; j < num; j++) {
                if (i != j) {
                    auto const diff = positions[j] - positions[i];
                    double invDist3 = 1.0 / (diff.magSquared() * diff.mag());
                    accelerations[i] += diff * mus[j] * invDist3;
                }
            }
        }
        // Perform a half-step
        for (size_t i = 0; i < num; i++) {
            velocities[i] += accelerations[i] * FACTOR / 2.0;
            positions[i] += velocities[i] * FACTOR;
        }
        // Update acceleration again

        for (size_t i = 0; i < num; i++) {
            accelerations[i] = Vec3D<double>();
        }
        for (size_t i = 0; i < num; i++) {
            for (size_t j = 0; j < num; j++) {
                if (i != j) {
                    auto const diff = positions[j] - positions[i];
                    double invDist3 = 1.0 / (diff.magSquared() * diff.mag());
                    accelerations[i] += diff * mus[j] * invDist3;
                }
            }
        }
        // Another half-step
        for (size_t i = 0; i < num; i++) {
            velocities[i] += accelerations[i] * (FACTOR / 2.0);
        }
        #pragma endregion Computation Loop
    }
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}