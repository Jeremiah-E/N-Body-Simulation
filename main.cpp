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

// The number of triangles in each circle
#define RESOLUTION 16
// The multiplier for the simulation's speed. Will be redone later
#define FACTOR 60
// The multiplier for the zoom effectiveness
#define ZOOM_FACTOR (double)(1.0 / 10.0)

// Precomputed sine and cosine values for generating circles
static double sines[RESOLUTION];
static double cosines[RESOLUTION];
static bool sincosInit = false;

// Camera constants
static double const fov = 2.0943951; // 120° in radians
static double const projectionScale = 1.0 / tan(fov / 2.0); // ~0.577
// Camera variables
static double camDist = 19640000 * 1.1; // The distance from the cIdxth body
static double pitch = 0; // Determines which angle the camera points at cIdx from
static double yaw = 0;   // Determines which angle the camera points at cIdx from
static size_t cIdx = 0; // The camera is camDist away from {positions[cIdx*3+0], positions[cIdx*3+1], positions[cIdx*3+2]}
static float minScale = 3; // The minimum size

// 3D vector of any arithmetic type
template <typename T> struct Vec3D {
    // Compile-time assert statement to ensure that T is something that allows math operations
    static_assert(std::is_arithmetic_v<T>, "T is not an arithmetic type");

    T x; T y; T z;
    // Default constructor, all zeroes
    Vec3D() : x(0), y(0), z(0) {};
    // Constructor given a T[3]
    Vec3D(const T var[3]) : x(var[0]), y(var[1]), z(var[2]) {}
    // Constructor given three literals
    Vec3D(T px, T py, T pz) : x(px), y(py), z(pz) {}    
    // Addition
    Vec3D operator+(Vec3D const &v) const {
        return {x + v.x, y + v.y, z + v.z};
    }
    // Scalar multiplication
    Vec3D operator*(T s) const {
        return {x * s, y * s, z * s};
    }
    // The other scalar multiplication
    friend Vec3D operator*(T s, const Vec3D& v) {
        return {v.x * s, v.y * s, v.z * s};
    }
    // Dot product
    T operator*(Vec3D const &v) const {
        return x * v.x + y * v.y + z * v.z;
    }
    // Cross Product
    Vec3D operator^(Vec3D const &v) const {
        return {y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x};
    }
    // Vector additive assignment
    Vec3D operator+=(Vec3D const &v) {
        x += v.x;
        y += v.y;
        z += v.z;
        return *this;
    }
    // Scalar multiplicative assignment
    Vec3D operator*=(T const s) {
        x *= s;
        y *= s;
        z *= s;
        return *this;
    }
    // Vector subtraction
    Vec3D operator-(Vec3D const &v) const {
        return Vec3D(x - v.x, y - v.y, z - v.z);
    }
    // Unary negation
    Vec3D operator-() const {
        return Vec3D(-x, -y, -z);
    }
    // Squared magnitude
    T mag2() const {
        return x * x + y * y + z * z;
    }
    // Magnitude (length)
    T mag() const {
        using FloatingType = std::common_type_t<T, double>;
        FloatingType val = std::sqrt(static_cast<FloatingType>(mag2()));
        return static_cast<T>(val);
    }
    // Normalize operator
    Vec3D norm() const {
        T length = mag();
        if (length == 0) return {0, 0, 0};

        using FloatingType = std::common_type_t<T, double>;
        FloatingType invMag = 1.0 / static_cast<FloatingType>(length);

        return {
            static_cast<T>(x * invMag),
            static_cast<T>(y * invMag),
            static_cast<T>(z * invMag)
        };
    }
};

// To flip the y axis. Might change this later to align with some plane
static const Vec3D<double> worldUp = {0, -1, 0};

// Given an array of centers, store the points of the circle in the vertex array
void createCircles(size_t num, float *centers, SDL_Vertex *verts, int *idxs, float *sizes, double *newNum) {
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

    // Filter out entries with size == 0 and compact the arrays
    size_t validIdx = 0;
    for (size_t i = 0; i < num; i++) {
        if (sizes[i] == -1) {
            (*newNum)--;
        } else {
            // Move this entry to validIdx position
            if (validIdx != i) {
                centers[validIdx * 2 + 0] = centers[i * 2 + 0];
                centers[validIdx * 2 + 1] = centers[i * 2 + 1];
                sizes[validIdx] = sizes[i];
            }
            validIdx++;
        }
    }

    // We assume verts is [resolution+1] times the length of newNum
    // With that, we go center by center and mutate verts
    // Since each iteration does not mess with the last, I hint the compiler parallelize it
    #pragma loop(hint_parallel(0))
    for (size_t i = 0; i < (size_t)*newNum; i++) {
        double cx = centers[i * 2 + 0];
        double cy = centers[i * 2 + 1];
        // Our local verts array
        SDL_Vertex *_verts = verts + (RESOLUTION + 1) * i;
        for (size_t j = 0; j < RESOLUTION; j++) {
            // The jth point along a circle
            _verts[j].position = {(float)(cx + sines[j] * sizes[i]), (float)(cy + cosines[j] * sizes[i])};
        }
        // The center of the circle, all tris forming the circle point here
        _verts[RESOLUTION].position = {(float)cx, (float)cy};
    }

    // Clear the color to red
    const size_t numVert = (size_t)*newNum * (RESOLUTION + 1);
    for (size_t i = 0; i < numVert; i++) {
        verts[i].color = {1, 0, 0, 1};
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
    std::unique_ptr<double []> radii;
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
        radii = std::make_unique<double[]>(num * 3);
        // Load positions
        for (int i = 0; i < num * 3; i++) {
            universe.read(reinterpret_cast<char*>(&positions[i]), sizeof(double));
        }
        // Load velocities
        for (int i = 0; i < num * 3; i++) {
            universe.read(reinterpret_cast<char*>(&velocities[i]), sizeof(double));
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

    // Create the window
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Pluto System", w, h, SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    SDL_SetRenderVSync(renderer, 1);
    
    // Draw loop variables
    double scale = 1.0 / 2.0 / (17527090.2 * 1.1);

    // Arrays used for various things inside the draw loop
    // Any array here must be overwritten before being read from
    auto sizes = std::make_unique<float[]>(num);
    auto centers = std::make_unique<float[]>(num*2); // Screen translated coords
    auto verts = std::make_unique<SDL_Vertex[]>((RESOLUTION + 1) * num);
    auto idxs = std::make_unique<int[]>(num*RESOLUTION*3);

    bool running = true;
    while (running) {
        // Gets the current events
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) { // The user/computer asked the program to quit
                // Terminate the loop
                running = false;
            }
            if (e.type == SDL_EVENT_MOUSE_WHEEL && e.wheel.y != 0) { // The mouse wheel scrolled vertically
                camDist *= 1 + e.wheel.y * ZOOM_FACTOR;
            } 
            if (e.type == SDL_EVENT_MOUSE_MOTION && (e.motion.state & SDL_BUTTON_LMASK)) {
                yaw += e.motion.xrel * (1.0/128.0);
                pitch += e.motion.yrel * (1.0/512.0);
            }
        }

        // Reset the screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Determine scaling based on the window's size
        int windowWidth;
        int windowHeight;
        SDL_GetWindowSize(window, &windowWidth, &windowHeight);
        
        // Get what the camera's pointing at
        Vec3D<double> cameraPos(positions.get() + cIdx * 3);
        // Get the camera position
        double cosPitch = cos(pitch);
        double sinPitch = sin(pitch);
        double cosYaw = cos(yaw);
        double sinYaw = sin(yaw);
        // Get the camera's actual position
        Vec3D<double> offset(cosPitch * sinYaw, sinPitch, cosPitch * cosYaw);
        offset *= camDist;
        cameraPos += offset;
        // forwards = -|offset|
        auto forwards = -offset.norm();
        // right = |forwards x worldUp|
        auto right = (forwards ^ worldUp).norm();
        // up = |right x forwards|
        auto up = (right ^ forwards).norm();

        // Set up the circle
        for (size_t i = 0; i < num; i++) {
            // The new, perspective method
            Vec3D<double> worldPos(positions.get() + i * 3);
            Vec3D<double> relPos = worldPos - cameraPos;

            double xView = relPos * right;
            double yView = relPos * up;
            double zView = relPos * forwards;

            if (zView > 0) {
                double projFactor = (windowHeight / 2.0) * projectionScale / zView;
                centers[i * 2 + 0] = (float)(xView * projFactor + windowWidth / 2.0);
                centers[i * 2 + 1] = (float)(yView * projFactor + windowHeight / 2.0);
                sizes[i] = max((float)(radii[i] * projFactor), minScale);
            } else {
                sizes[i] = -1;
            }
            // The old, orthogonal method
            // centers[i * 2 + 0] = (float)(positions[i * 3 + 0] * screenScale + windowWidth  / 2.0);
            // centers[i * 2 + 1] = (float)(positions[i * 3 + 1] * screenScale + windowHeight / 2.0);
            // sizes[i] = 10;
        }
        double newNum = num;
        createCircles(num, centers.get(), verts.get(), idxs.get(), sizes.get(), &newNum);
        
        // Draw the circles
        SDL_RenderGeometry(renderer, NULL, verts.get(), (RESOLUTION + 1) * newNum, idxs.get(), newNum * RESOLUTION * 3);

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