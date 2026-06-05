#include "OrbitalDebris.h"

namespace OrbitPropagator {

    double SolveKepler(double M, double e) {
        double E = M;
        const double tolerance = 1e-6;
        const int maxIterations = 30;

        for (int idx = 0; idx < maxIterations; ++idx) {
            double deltaE = (E - e * std::sin(E) - M) / (1.0 - e * std::cos(E));
            E -= deltaE;
            if (std::abs(deltaE) < tolerance) {
                break;
            }
        }
        return E;
    }

    Vector3 Propagate(DebrisObject& object, double simTime) {
        // Mean Anomaly at sim time t
        double M = object.ma + object.n * simTime;

        // Solve Kepler's equation
        double E = SolveKepler(M, object.e);

        // Calculate Position coordinates in orbital plane (in PQW coordinate frame)
        double xOrb = object.a * (std::cos(E) - object.e);
        double yOrb = object.a * std::sqrt(1.0 - object.e * object.e) * std::sin(E);

        // Coordinate transformation: Orbital Plane (PQW) to Earth-Centered Inertial (ECI)
        double cosRaan = std::cos(object.raan);
        double sinRaan = std::sin(object.raan);
        double cosArg = std::cos(object.arg);
        double sinArg = std::sin(object.arg);
        double cosI = std::cos(object.i);
        double sinI = std::sin(object.i);

        // Euler angle rotations (Raan -> I -> Arg)
        double x = xOrb * (cosRaan * cosArg - sinRaan * sinArg * cosI) - yOrb * (cosRaan * sinArg + sinRaan * cosArg * cosI);
        double y = xOrb * (sinRaan * cosArg + cosRaan * sinArg * cosI) - yOrb * (sinRaan * sinArg - cosRaan * cosArg * cosI);
        double z = xOrb * (sinArg * sinI) + yOrb * (cosArg * sinI);

        // Store dynamic calculations
        object.position = Vector3(x, y, z);
        
        double distance = object.position.length();
        object.altitude = distance - EARTH_RADIUS_KM;
        
        // Vis-Viva Equation: v = sqrt(mu * (2/r - 1/a))
        object.velocity = std::sqrt(MU * (2.0 / distance - 1.0 / object.a));

        return object.position;
    }
}
