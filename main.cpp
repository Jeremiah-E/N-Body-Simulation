#include <windows.h>
#include <SDL3/SDL.h>

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

        // Draw a triangle
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Define the points
        SDL_Point p1 = {400, 100};
        SDL_Point p2 = {200, 500};
        SDL_Point p3 = {600, 500};

        // Put the points into a vertex array
        SDL_Vertex verts[3];
        verts[0].position.x = (float)p1.x; verts[0].position.y = (float)p1.y; 
        verts[1].position.x = (float)p2.x; verts[1].position.y = (float)p2.y; 
        verts[2].position.x = (float)p3.x; verts[2].position.y = (float)p3.y; 
        verts[0].color = {1.0f, 0.0f, 0.0f, 1.0f};
        verts[1].color = {0.0f, 1.0f, 0.0f, 1.0f}; // Red, green, then blue
        verts[2].color = {0.0f, 0.0f, 1.0f, 1.0f};
        verts[0].tex_coord = {0.0f, 0.0f};
        verts[1].tex_coord = {0.0f, 0.0f};
        verts[2].tex_coord = {0.0f, 0.0f};

        // Actually draw the triangle
        int indices[3] = {0, 1, 2};
        SDL_RenderGeometry(renderer, NULL, verts, 3, indices, 3);

        SDL_RenderPresent(renderer);
    }

    // Some memory management
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}