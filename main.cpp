#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <type_traits>
#include <windows.h>

#include <SDL3/SDL.h>
#include "grav.hpp" // Imports the other hpp files

#pragma region Macros
// The number of triangles in each circle
#define RESOLUTION 20
// The multiplier for the simulation's speed. Will be redone later
#define FACTOR 10
// The multiplier for the zoom effectiveness
#define ZOOM_FACTOR (double)(1.0 / 10.0)
// Which file to load data from
#define DATA_NAME "universe.bin"
#pragma endregion Macros

#pragma region Global Variables
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
static Vec3D<double> camForw = {0, 0, 1};
static Vec3D<double> camUp = {0, 1, 0};
static Vec3D<double> camRight = {1, 0, 0};
#pragma endregion Global Variables

#pragma region Circle Drawing
// Given an array of centers, store the points of the circle in the vertex array
void createCircles(int *num, std::vector<Vec3D<float>> centers, SDL_Vertex *verts, int *idxs, float *sizes, const size_t _cIdx) {
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

    // To prevent me from looping over a changing variable
    int oldNum = *num;

    // Filter out entries with invalid size and compact the arrays
    // This culls shapes off screen, given how this function is used in the draw loop
    size_t validIdx = 0;
    size_t postCullCIdx = _cIdx;
    bool isCIdxCulled = false;
    for (size_t i = 0; i < oldNum; i++) {
        if (sizes[i] <= 0.0) {
            // i was invalid, so we know there's one less valid object
            (*num)--;
            // If we get here, sizes[cIdx] was invalid
            if (i == _cIdx) { isCIdxCulled = true; }
        } else {
            // Move cIdx as needed
            if (i == _cIdx) {
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
    std::vector<size_t> sortedIndices(*num);
    for (size_t i = 0; i < *num; i++) {
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
    std::vector<Vec3D<float>> tempCenters(*num);
    std::vector<float> tempSizes(*num);
    for (size_t i = 0; i < *num; i++) {
        tempCenters[i] = centers[sortedIndices[i]];
        tempSizes[i] = sizes[sortedIndices[i]];
    }
    // Insert the temp arrays back
    for (size_t i = 0; i < *num; i++) {
        centers[i] = tempCenters[i];
        sizes[i] = tempSizes[i];
    }
    // Store the old indices
    std::vector<size_t> inverseSortedIndices(*num);
    for (size_t i = 0; i < *num; i++) {
        inverseSortedIndices[sortedIndices[i]] = i;
    }

    // We assume verts is [resolution+1] times the length of newNum
    // With that, we go center by center and mutate verts
    // Since each iteration does not mess with the last, I hint the compiler parallelize it
    #pragma loop(hint_parallel(0))
    for (size_t i = 0; i < (size_t)*num; i++) {
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
    const size_t numVert = (size_t)*num * (RESOLUTION + 1);
    size_t finalCIdx = isCIdxCulled ? (size_t)-1 : inverseSortedIndices[postCullCIdx];
    for (size_t i = 0; i < numVert; i++) {
        if (!isCIdxCulled && (i / (RESOLUTION + 1) == finalCIdx)) {
            verts[i].color = {0, 0, 1, 1}; // Blue
        } else {
            verts[i].color = {1, 0, 0, 1}; // Red
        }
    }

    // Build triangle indices for each circle
    for (size_t i = 0; i < (size_t)*num; i++) {
        size_t base = (RESOLUTION + 1) * i;
        size_t ibase = RESOLUTION * 3 * i;
        for (size_t j = 0; j < RESOLUTION; j++) {
            size_t tri = ibase + j * 3;
            idxs[tri + 0] = (int)(base + j);
            idxs[tri + 1] = (int)(base + ((j + 1) % RESOLUTION));
            idxs[tri + 2] = (int)(base + RESOLUTION);
        }
    }
}
#pragma endregion Circle Drawing

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    // Make the compiler stop complaining
    (void)nShowCmd;
    (void)lpCmdLine;
    (void)hInstance;
    (void)hPrevInstance;
    // Initialize data
    int num;
    std::vector<Vec3D<double>> positions;
    std::vector<Vec3D<double>> velocities;
    std::vector<Vec3D<double>> accelerations;
    std::vector<double> mus;
    std::vector<double> radii;
    std::vector<std::string> names;
    // Import data from DATA_NAME
    import(DATA_NAME, &num, &positions, &velocities, &accelerations, &mus, &radii, &names);

    #pragma region Create Window
    SDL_Init(SDL_INIT_VIDEO);
    // Get monitor details
    int count;
    SDL_DisplayID* displays = SDL_GetDisplays(&count);
    const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(displays[0]);
    SDL_free(displays);
    // Now create the window
    SDL_Window* window = SDL_CreateWindow("Solar System", (int)(mode->w / 2.0), (int)(mode->h / 2.0), SDL_WINDOW_RESIZABLE);
    // Move it to the desired location, centering it on the screen
    SDL_SetWindowPosition(window, (int)(mode->w / 4.0), (int)(mode->h / 4.0));
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
            // The user/computer asked the program to quit
            if (e.type == SDL_EVENT_QUIT) {
                // Terminate the loop
                running = false;
            }
            // The mouse wheel scrolled vertically
            if (e.type == SDL_EVENT_MOUSE_WHEEL && e.wheel.y != 0) {
                camDist *= 1 - e.wheel.y * ZOOM_FACTOR;
            } 
            // The mouse dragged
            if (e.type == SDL_EVENT_MOUSE_MOTION && (e.motion.state & SDL_BUTTON_LMASK)) {
                // Get how much to rotate everything by
                double yaw = e.motion.xrel * 0.008;
                double pitch = e.motion.yrel * 0.005;
                // Rotate everything
                camForw = camForw.rotate(GLOBAL_UP, yaw);
                camRight = camRight.rotate(GLOBAL_UP, yaw);
                camForw = camForw.rotate(camRight, pitch);
                // Normalize and recalculate everything
                camForw = camForw.norm(); // Renormalize forwards (floating point drift)
                camRight = (camRight - camForw * (camRight * camForw)).norm(); // Reorthogonalize right
                camUp = (camRight ^ camForw).norm(); // Recompute up
            }
            // A key was pressed
            if (e.type == SDL_EVENT_KEY_DOWN) {
                // Left bracket
                if (e.key.key == SDLK_LEFTBRACKET) {
                    cIdx = (cIdx - 1 + num) % num;
                }
                // Right bracket
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

        #pragma region Projection
        // Get the width/height for the centering of everything
        int windowWidth;
        int windowHeight;
        SDL_GetWindowSize(window, &windowWidth, &windowHeight);
        
        // The camera is pointing at positions[cIdx] at distance camDist
        Vec3D<double> cameraPos = positions[cIdx] - camDist * camForw;

        // Set up the circle
        for (size_t i = 0; i < num; i++) {
            // The new, perspective method
            const Vec3D<double> worldPos = positions[i] - cameraPos;

            const double zView = worldPos * camForw; // Used for culling, so it gets calculated first
            
            if (zView > 0) {
                const double xView = worldPos * camRight;
                const double yView = worldPos * camUp;

                const double projFactor = (windowHeight / 2.0) * projectionScale / zView;
                
                centers[i].x = (float)(xView * projFactor + windowWidth / 2.0);
                centers[i].y = (float)(yView * projFactor + windowHeight / 2.0);
                centers[i].z = (float)zView; // Used for draw order

                sizes[i] = max((float)(radii[i] * projFactor), minScale);
            } else {
                sizes[i] = -1;
            }
        }
        // Draw all circles
        int newNum = num; createCircles(&newNum, centers, verts.get(), idxs.get(), sizes.get(), cIdx);
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
        
        integrate(&positions, &velocities, &mus, FACTOR);
    }
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}