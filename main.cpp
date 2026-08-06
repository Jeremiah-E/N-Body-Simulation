#include <windows.h>
#include <SDL3/SDL.h>

// The number of triangles in each circle
#define resolution 8

// Precomputed sine and cosine values for generating circles
static double sines[resolution];
static double cosines[resolution];
static bool sincosInit = false;

// Given an array of centers, store the points of the circle in the vertex array
void createCircles(size_t num, double *centers, SDL_Vertex *verts, int *idxs, double size) {
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

// A "hello, world" program of sorts
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Hello, world!", 800, 600, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    SDL_SetRenderVSync(renderer, 1);

    bool running = true;
    while (running) {
        // Gets the current events
        SDL_Event e;
        // Runs while there's an event
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                // Terminate the loop
                running = false;
            }
        }

        // Reset the screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        // Set up the circle
        double centers[2*2] = {100, 30, 650, 200};
        SDL_Vertex verts[(resolution + 1) * 2] = {0};
        int idxs[2 * resolution * 3] = {0};
        createCircles(2, centers, verts, idxs, 10);

        // Actually draw the circles
        SDL_RenderGeometry(renderer, NULL, verts, (resolution + 1) * 2, idxs, 2 * resolution * 3);

        SDL_RenderPresent(renderer);
    }

    // Some memory management
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}