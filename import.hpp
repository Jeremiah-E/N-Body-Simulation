#ifndef READ_HPP
#define READ_HPP
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

#include <SDL3/SDL.h>
#include "vector.hpp"
#define NBODY_VER "NBS1" // N-Body Sim. v1

// Reads the binary file into the given variables
void import(string datapath, int *num, vector<Vec3D<double>> *positions, vector<Vec3D<double>> *velocities, vector<Vec3D<double>> *accelerations, vector<double> *mus, vector<double> *radii, vector<string> *names) {
    // Read from universe.bin
    ifstream universe(datapath, ios::binary);
    if (universe.is_open()) {
        // Get the version
        string version = "";
        getline(universe, version, '\0');
        // The magic number did not match
        if (version.compare(NBODY_VER) != 0) {
            // Check that version is printable
            bool canPrintVer = true;
            for (const char &ch : version) {
                canPrintVer &= isprint(ch);
            }
            // Assemble the message
            string message = "Invalid version provided. ";
            if (canPrintVer) {
                message += "Expected to see version ";
                message += NBODY_VER;
                message += ", found ";
                message += version;
                message += ".";
            } else {
                message += "Unidentifiable/unprintable version in file.";
            }
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "NBody.exe - Critical Error", message.c_str(), NULL);
            exit(EXIT_FAILURE);
        }
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
            string name;
            // Get up to the null-terminator
            getline(universe, name, '\0');
            // Add it to names
            (*names).push_back(move(name));
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