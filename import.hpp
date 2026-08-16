#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "vector.hpp"

#ifndef READ_HPP
#define READ_HPP
void import(std::string datapath, int *num, std::vector<Vec3D<double>> *positions, std::vector<Vec3D<double>> *velocities, std::vector<Vec3D<double>> *accelerations, std::vector<double> *mus, std::vector<double> *radii, std::vector<std::string> *names) {
    // Read from universe.bin
    std::ifstream universe(datapath, std::ios::binary);
    if (universe.is_open()) {
        // Get num
        universe.read(reinterpret_cast<char*>(num), sizeof(int));
        // Create the pointer arrays/vector for everything
        (*positions).resize(*num);
        (*velocities).resize(*num);
        (*accelerations).resize(*num);
        (*mus).resize(*num);
        (*names).reserve(*num);
        (*radii).resize(*num);
        // Load positions
        for (int i = 0; i < *num; i++) {
            (*positions)[i] = Vec3D<double>();
            universe.read(reinterpret_cast<char*>(&((*positions)[i].x)), sizeof(double));
            universe.read(reinterpret_cast<char*>(&((*positions)[i].y)), sizeof(double));
            universe.read(reinterpret_cast<char*>(&((*positions)[i].z)), sizeof(double));
        }
        // Load velocities
        for (int i = 0; i < *num; i++) {
            (*velocities)[i] = Vec3D<double>();
            universe.read(reinterpret_cast<char*>(&((*velocities)[i].x)), sizeof(double));
            universe.read(reinterpret_cast<char*>(&((*velocities)[i].y)), sizeof(double));
            universe.read(reinterpret_cast<char*>(&((*velocities)[i].z)), sizeof(double));
        }
        // Load mus
        for (int i = 0; i < *num; i++) {
            universe.read(reinterpret_cast<char*>(&(*mus)[i]), sizeof(double));
        }
        // Load names
        for (int i = 0; i < *num; i++) {
            std::string name;
            // Get up to the null-terminator
            std::getline(universe, name, '\0');
            // Add it to names
            (*names).push_back(std::move(name));
        }
        // Load radii
        for (int i = 0; i < *num; i++) {
            universe.read(reinterpret_cast<char*>(&(*radii)[i]), sizeof(double));
        }
        
        universe.close();
    } else {
        // Could not open the file
        throw 1;
    }
}
#endif