#ifndef GRAV_HPP
#define GRAV_HPP
#include "import.hpp"

using namespace std;

// A fairly niave computation of the gravity pulling on every body
//
// Given Newton's gravity formula (a=GM/|r|² • r.norm()), we can comptute the gravitational pull of everything in O(n²)
//
// The only optimization here is skipping half of the loop, calculating the two-way pull and only iterating over each pair. Given n=47, this suffices for the solar system simulation.
//
// Note: will probaly not reuse this for mission simulation/planning due to large a large n.
void updateAccels(vector<Vec3D<double>> positions, vector<Vec3D<double>> *accelerations, vector<double> mus) {
    // Clear acceleration
    for (size_t i = 0; i < (*accelerations).size(); i++) {
        (*accelerations)[i] = {0, 0, 0};
    }
    // Loop through each position
    for (size_t i = 0; i < positions.size(); i++) {
        // Loop through again
        for (size_t j = 0; j < i; j++) {
            // Compute j pulling on i
            Vec3D<double> force = (positions[j] - positions[i]).norm();
            force /= positions[i].distSquared(positions[j]);
            force *= mus[j];
            (*accelerations)[i] += force;
            // Compute i pulling on j
            force /= mus[j];
            force *= mus[i];
            (*accelerations)[j] -= force;
        }
    }
};

// Verlet integration to move the simulation forward by dt
void integrate(vector<Vec3D<double>> *positions, vector<Vec3D<double>> *velocities, vector<double> *mus, double dt) {
    vector<Vec3D<double>> accelerations;
    accelerations.resize((*positions).size());

    // positions += velocity * dt / 2
    // velocity += acceleration * dt
    // positions += velocity * dt / 2

    updateAccels(*positions, &accelerations, *mus);
    for (size_t i = 0; i < (*positions).size(); i++) {
        (*positions)[i] += (*velocities)[i] / 2.0 * dt;
    }
    updateAccels(*positions, &accelerations, *mus);
    for (size_t i = 0; i < (*positions).size(); i++) {
        (*velocities)[i] += accelerations[i] * dt;
        (*positions)[i] += (*velocities)[i] / 2.0 * dt;
    }
}
#endif