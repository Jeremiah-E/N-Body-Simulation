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

#pragma region Constants

// The number of triangles in each circle
#define RESOLUTION 16
// The multiplier for the simulation's speed. Will be redone later
#define FACTOR 360
// The multiplier for the zoom effectiveness
#define ZOOM_FACTOR (double)(1.0 / 10.0)

// Precomputed sine and cosine values for generating circles
static double sines[RESOLUTION];
static double cosines[RESOLUTION];
static bool sincosInit = false;

// Camera constants
static double const fov = 1.5707963; // 90°
static double const projectionScale = 1.0 / tan(fov / 2.0); // Exactly 1, but we keep it here incase we want to change fov
// Camera variables
static double camDist = 19640000 * 1.1; // The distance from the cIdxth body
static double pitch = 0; // Determines which angle the camera points at cIdx from
static double yaw = 0;   // Determines which angle the camera points at cIdx from
static size_t cIdx = 0; // The camera is camDist away from {positions[cIdx*3+0], positions[cIdx*3+1], positions[cIdx*3+2]}
static float minScale = 3; // The minimum size of a body. When textures are introduced, this will determine when something's a point

#pragma endregion Constants

#pragma region Vector
// 3D vector of any arithmetic type
template <typename T> struct Vec3D {
    // Compile-time assert statement to ensure that T is something that allows math operations
    static_assert(std::is_arithmetic_v<T>, "T is not an arithmetic type");
    // The components of the vector. Only makes sense when in some reference frame, which will be derived from the dataset
    T x; T y; T z;
    // Default constructor, all zeroes
    Vec3D() : x(0), y(0), z(0) {};
    // Constructor given T [3]
    Vec3D(const T var[3]) : x(var[0]), y(var[1]), z(var[2]) {}
    // Constructor given three literals
    Vec3D(T px, T py, T pz) : x(px), y(py), z(pz) {}
    // Constructor given T *
    // Ensure the pointer has enough room to call var[2]
    Vec3D(T *var) : Vec3D(var[0], var[1], var[2]){}
    // Addition
    Vec3D operator+(Vec3D const &v) const {
        return {x + v.x, y + v.y, z + v.z};
    }
    // Scalar multiplication
    Vec3D operator*(const T s) const {
        return {x * s, y * s, z * s};
    }
    // Scalar multiplicative assignment
    void operator*=(const T s) {
        x *= s; y *= s; z *= s;
    }
    // Multiplication by a scalar
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
    // Cross product assignment
    void operator^=(Vec3D const v) {
        double xp = y * v.z - z * v.y;
        double yp = z * v.x - x * v.z;
        double zp = x * v.y - y * v.x;
        x = xp; y = yp; z = zp;
    }
    // Vector addition
    Vec3D operator+(Vec3D const &v) {
        return {x + v.x, y + v.y, z + v.z};
    }
    // Vector additive assignment
    void operator+=(Vec3D const &v) {
        x += v.x; y += v.y; z += v.z;
    }
    // Vector subtraction
    Vec3D operator-(Vec3D const &v) const {
        return {x - v.x, y - v.y, z - v.z};
    }
    // Vector subtractive assignment
    void operator-=(Vec3D const &v) {
        x -= v.x; y -= v.y; z -= v.z;
    }
    // Unary negation
    Vec3D operator-() const {
        return Vec3D(-x, -y, -z);
    }
    // Squared magnitude
    // Exists incase we end up using this for gravity math, where it's slightly optimal to do mag() * magSquared()
    T magSquared() const {
        return x * x + y * y + z * z;
    }
    // Magnitude (length)
    T mag() const {
        // Promotes the type to a double
        using FloatingType = std::common_type_t<T, double>;
        // Since it's so wordy:
        // val = sqrt(magSquared)
        FloatingType val = std::sqrt(static_cast<FloatingType>(magSquared()));
        return static_cast<T>(val);
    }
    // Normalize operator
    Vec3D norm() const {
        const T length = mag();
        // Fallback incase it's already zero
        if (length == 0) return {0, 0, 0};
        // Promotion to double
        using FloatingType = std::common_type_t<T, double>;
        FloatingType invMag = 1.0 / static_cast<FloatingType>(length);
        // Cast it back to T after we use double precision
        return {
            static_cast<T>(x * invMag),
            static_cast<T>(y * invMag),
            static_cast<T>(z * invMag)
        };
    }
    // Scalar comparison: greater than
    // Equivalent to v.mag() > m
    bool operator>(const T m) const {
        return mag() > m;
    }
    // Scalar comparison: greater than or equal to
    // Equivalent to v.mag() >= m
    bool operator>=(const T m) const {
        return mag() >= m;
    }
    // Scalar comparison: less than
    // Equivalent to v.mag() < m
    bool operator<(const T m) const {
        return mag() < m;
    }
    // Scalar comparison: less than or equal to
    // Equivalent to v.mag() <= m
    bool operator<=(const T m) const {
        return mag() <= m;
    }
    // Scalar division
    Vec3D operator/(const T m) const {
        using FloatingType = std::common_type_t<T, double>;
        FloatingType id = 1.0 / static_cast<FloatingType>(m);
        return {
            static_cast<T>(x * id),
            static_cast<T>(y * id),
            static_cast<T>(z * id)
        };
    }
    // Scalar division assignment
    void operator/=(const T m) {
        using FloatingType = std::common_type_t<T, double>;
        FloatingType id = 1.0 / static_cast<FloatingType>(m);
        x = static_cast<T>(x * id);
        y = static_cast<T>(y * id);
        z = static_cast<T>(z * id);
    }
    // Distance squared
    T distSquared(Vec3D const v) const {
        return (this - v).magSquared();
    }
    // Distance
    T dist(Vec3D const &v) const {
        using FloatingType = std::common_type_t<T, double>;
        FloatingType d = std::sqrt(static_cast<FloatingType>(distSquared(v)));
        return static_cast<T>(d);
    }
    // Equality
    // Warning: floating point precision applies here
    bool operator==(Vec3D const &v) const {
        return x == v.x && y == v.y && z == v.z;
    }
    // Inequality
    // Warning: floating point precision applies here
    bool operator!=(Vec3D const &v) const {
        return !(*this == v);
    }
};

// To flip the y axis. Completely arbitrary, might align to the ecliptic, depending on how the data's structured
static const Vec3D<double> worldUp = Vec3D<double>(0.0, -1.0, 0.0).norm();
#pragma endregion Vector

#pragma region Circle Drawing
// Given an array of centers, store the points of the circle in the vertex array
void createCircles(const size_t num, float *centers, SDL_Vertex *verts, int *idxs, float *sizes, double *newNum, const size_t cIdx) {
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
                centers[validIdx * 3 + 0] = centers[i * 3 + 0];
                centers[validIdx * 3 + 1] = centers[i * 3 + 1];
                centers[validIdx * 3 + 2] = centers[i * 3 + 2];
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
    std::sort(sortedIndices.begin(), sortedIndices.end(), [&centers](size_t a, size_t b) {return centers[a * 3 + 2] > centers[b * 3 + 2];});
    // Store everything in temp arrays according to sortedIndices
    std::vector<float> tempCenters(*newNum * 3);
    std::vector<float> tempSizes(*newNum);
    for (size_t i = 0; i < *newNum; i++) {
        tempCenters[i * 3 + 0] = centers[sortedIndices[i] * 3 + 0];
        tempCenters[i * 3 + 1] = centers[sortedIndices[i] * 3 + 1];
        tempCenters[i * 3 + 2] = centers[sortedIndices[i] * 3 + 2];
        tempSizes[i] = sizes[sortedIndices[i]];
    }
    // Insert the temp arrays back
    for (size_t i = 0; i < *newNum; i++) {
        centers[i * 3 + 0] = tempCenters[i * 3 + 0];
        centers[i * 3 + 1] = tempCenters[i * 3 + 1];
        centers[i * 3 + 2] = tempCenters[i * 3 + 2];
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
        double cx = centers[i * 3 + 0];
        double cy = centers[i * 3 + 1];
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
    #pragma endregion File Loading

    #pragma region Create Window
    SDL_Init(SDL_INIT_VIDEO);
    // Get monitor details
    int count;
    SDL_DisplayID* displays = SDL_GetDisplays(&count);
    const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(displays[0]);
    SDL_free(displays);
    // Now create the window
    SDL_Window* window = SDL_CreateWindow("Pluto System", mode->w / 2.0, mode->h / 2.0, SDL_WINDOW_RESIZABLE);
    // Move it to the desired location, centering it on the screen
    SDL_SetWindowPosition(window, mode->w / 4.0, mode->h / 4.0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    SDL_SetRenderVSync(renderer, 1);
    #pragma endregion Create Window
    
    // Arrays used for various things inside the draw loop
    // Any array here must be overwritten before being read from
    auto sizes = std::make_unique<float[]>(num);
    auto centers = std::make_unique<float[]>(num*3); // Screen translated coords
    auto verts = std::make_unique<SDL_Vertex[]>((RESOLUTION + 1) * num);
    auto idxs = std::make_unique<int[]>(num*RESOLUTION*3);

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
                camDist *= 1 + e.wheel.y * ZOOM_FACTOR;
            } 
            if (e.type == SDL_EVENT_MOUSE_MOTION && (e.motion.state & SDL_BUTTON_LMASK)) {
                yaw += e.motion.xrel * (1.0/128.0);
                pitch += e.motion.yrel * (1.0/512.0);
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
        
        // Get what the camera's pointing at
        // positions + cIdx + 3 points to teh 'cIdx'th planet's position, we effectively treat it as a double[3]
        Vec3D<double> cameraPos(positions.get() + cIdx * 3);
        // Get the offset of the camera to determine the camera's position
        Vec3D<double> offset(cos(pitch) * sin(yaw), sin(pitch), cos(pitch) * cos(yaw));
        offset *= camDist;
        // Finally get the camera's position
        cameraPos += offset;

        // Determine the three coordinate axes of the camera: forwards, right, and up
        // (Note: I use |v| to describe what's traditionally written as v/|v|, the normalization of a vector)
        // forwards = -|offset|
        // (Note: unsure the order of operations here, but -|x| = |-x|, so we don't care in this instance)
        const auto forwards = -offset.norm();
        // right = |forwards x worldUp|
        // (Note: worldUp is completely arbitrary, and not to be confused with up [the camera's coordinate system's 'up' axis])
        const auto right = (forwards ^ worldUp).norm();
        // up = |right x forwards|
        const auto up = (right ^ forwards).norm();
        #pragma endregion Camera

        #pragma region Projection
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
                centers[i * 3 + 0] = (float)(xView * projFactor + windowWidth / 2.0);
                centers[i * 3 + 1] = (float)(yView * projFactor + windowHeight / 2.0);
                centers[i * 3 + 2] = (float)(zView); // Used for culling / draw order
                sizes[i] = max((float)(radii[i] * projFactor), minScale);
            } else {
                sizes[i] = -1;
            }
            // The old, orthogonal method
            // Kept here for posterity and in-case perpective methods don't pan out
            // centers[i * 2 + 0] = (float)(positions[i * 3 + 0] * screenScale + windowWidth  / 2.0);
            // centers[i * 2 + 1] = (float)(positions[i * 3 + 1] * screenScale + windowHeight / 2.0);
            // sizes[i] = 10;
        }
        double newNum = num;
        createCircles(num, centers.get(), verts.get(), idxs.get(), sizes.get(), &newNum, cIdx);

        // Draw all circles in a single batched call
        SDL_RenderGeometry(renderer, NULL, verts.get(), newNum * (RESOLUTION + 1), idxs.get(), newNum * RESOLUTION * 3);
        #pragma endregion Projection

        #pragma region Text
        // Draw some text
        const size_t bufLen = 50;
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        char buf[bufLen];
        // Row 1: "Simulating 2 bodies"
        snprintf(buf, bufLen, "Simulating %d bodies", num);
        SDL_RenderDebugText(renderer, 10, 10, buf);
        // Row 2: "Pluto, Charon"
        size_t bufIdx = 0;
        // Append each name (barring the last) to buf
        for (size_t i = 0; i + 1 < static_cast<size_t>(num); i++) {
            snprintf(buf + bufIdx, bufLen - bufIdx, "%s, ", names[i].c_str());
            bufIdx += names[i].size() + 2;
        }
        // Append the last name to buf
        snprintf(buf + bufIdx, bufLen - bufIdx, "%s", names[num - 1].c_str());
        SDL_RenderDebugText(renderer, 10, 20, buf);
        // Row 3: "Centered: Pluto"
        snprintf(buf, bufLen, "Centered: %s", names[cIdx].c_str());
        SDL_RenderDebugText(renderer, 10, 30, buf);
        #pragma endregion Text

        SDL_RenderPresent(renderer);
        #pragma endregion Draw Loop

        #pragma region Computation Loop
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
        #pragma endregion Computation Loop
    }
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}