#ifndef GRAV_HPP
#define GRAV_HPP
#include "import.hpp"

using namespace std;

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