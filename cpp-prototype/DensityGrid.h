#ifndef DENSITY_GRID_H
#define DENSITY_GRID_H

#include "OrbitalDebris.h"
#include <vector>

struct Voxel {
    Vector3 center;
    float density;
    float color[3]; // RGB colors
};

class DensityGrid {
public:
    DensityGrid(int resolution = 32, double bound = 3.0);
    ~DensityGrid();

    // Rebuild grid bins from debris dataset
    void Update(const std::vector<DebrisObject>& debrisList);

    // Get calculated density at visual coordinates
    float GetDensityAt(const Vector3& visualPos) const;

    // Get list of active visual voxels (density > 0)
    std::vector<Voxel> GetActiveVoxels() const;

    int GetResolution() const { return m_resolution; }
    double GetBound() const { return m_bound; }
    double GetVoxelSize() const { return m_voxelSize; }

private:
    int m_resolution;
    double m_bound;
    double m_voxelSize;
    std::vector<float> m_gridData;
    
    // Internal coordinate helpers
    int GetVoxelIndex(int ix, int iy, int iz) const;
    void VisualToGrid(const Vector3& pos, int& ix, int& iy, int& iz) const;
};

#endif // DENSITY_GRID_H
