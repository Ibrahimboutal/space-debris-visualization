#ifndef ORBITAL_DEBRIS_H
#define ORBITAL_DEBRIS_H

#include <string>
#include <vector>
#include <cmath>

// Struct to represent 3D physical coordinates
struct Vector3 {
    double x, y, z;
    
    Vector3() : x(0.0), y(0.0), z(0.0) {}
    Vector3(double x, double y, double z) : x(x), y(y), z(z) {}
    
    double length() const {
        return std::sqrt(x*x + y*y + z*z);
    }
    
    Vector3 normalized() const {
        double len = length();
        if (len > 0.0) {
            return Vector3(x / len, y / len, z / len);
        }
        return Vector3();
    }
};

// Simplified Keplerian element representation of orbital objects
struct DebrisObject {
    int id;
    std::string name;
    
    // Orbital elements
    double a;    // Semi-major axis in km
    double e;    // Eccentricity
    double i;    // Inclination in radians
    double raan; // Right Ascension of Ascending Node in radians
    double arg;  // Argument of perigee in radians
    double ma;   // Mean anomaly at epoch (radians)
    double n;    // Mean motion (radians/sec)
    
    std::string regime;   // LEO, MEO, GEO
    std::string category; // sat, rocket, frag
    double size;          // diameter in meters
    
    // Runtime dynamic properties
    Vector3 position;
    double velocity;
    double altitude;
    double risk;
    bool visible;
};

// Keplerian Orbit propagation engine functions
namespace OrbitPropagator {
    // Gravitational constants
    const double MU = 398600.4418;       // Earth Standard Gravitational Parameter (km^3/s^2)
    const double EARTH_RADIUS_KM = 6371.0; // Earth mean radius in km

    // Solves Kepler's Equation E - e * sin(E) = M
    double SolveKepler(double M, double e);

    // Propagates orbit elements to a specific time and updates Cartesian coordinates
    Vector3 Propagate(DebrisObject& object, double simTime);
}

#endif // ORBITAL_DEBRIS_H
