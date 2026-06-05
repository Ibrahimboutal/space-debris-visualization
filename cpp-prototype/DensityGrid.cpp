#include "DensityGrid.h"
#include <algorithm>

DensityGrid::DensityGrid(int resolution, double bound)
    : m_resolution(resolution), m_bound(bound) {
    m_voxelSize = (m_bound * 2.0) / m_resolution;
    m_gridData.resize(m_resolution * m_resolution * m_resolution, 0.0f);
}

DensityGrid::~DensityGrid() {}

int DensityGrid::GetVoxelIndex(int ix, int iy, int iz) const {
    return ix + iy * m_resolution + iz * m_resolution * m_resolution;
}

void DensityGrid::VisualToGrid(const Vector3& pos, int& ix, int& iy, int& iz) const {
    ix = std::floor((pos.x + m_bound) / m_voxelSize);
    iy = std::floor((pos.y + m_bound) / m_voxelSize);
    iz = std::floor((pos.z + m_bound) / m_voxelSize);
}

void DensityGrid::Update(const std::vector<DebrisObject>& debrisList) {
    // Reset density grid data to zero
    std::fill(m_gridData.begin(), m_gridData.end(), 0.0f);

    const double scale = 1.0 / OrbitPropagator::EARTH_RADIUS_KM;

    for (const auto& deb : debrisList) {
        if (!deb.visible) continue;

        // Convert physical coordinates to OpenGL visual space
        Vector3 visualPos(deb.position.x * scale, deb.position.y * scale, deb.position.z * scale);

        // Check bounding volume
        if (std::abs(visualPos.x) < m_bound && 
            std::abs(visualPos.y) < m_bound && 
            std::abs(visualPos.z) < m_bound) {
            
            int ix, iy, iz;
            VisualToGrid(visualPos, ix, iy, iz);

            if (ix >= 0 && ix < m_resolution && 
                iy >= 0 && iy < m_resolution && 
                iz >= 0 && iz < m_resolution) {
                
                int index = GetVoxelIndex(ix, iy, iz);
                m_gridData[index] += 1.0f;
            }
        }
    }
}

float DensityGrid::GetDensityAt(const Vector3& visualPos) const {
    if (std::abs(visualPos.x) < m_bound && 
        std::abs(visualPos.y) < m_bound && 
        std::abs(visualPos.z) < m_bound) {
        
        int ix, iy, iz;
        VisualToGrid(visualPos, ix, iy, iz);
        
        if (ix >= 0 && ix < m_resolution && 
            iy >= 0 && iy < m_resolution && 
            iz >= 0 && iz < m_resolution) {
            return m_gridData[GetVoxelIndex(ix, iy, iz)];
        }
    }
    return 0.0f;
}

std::vector<Voxel> DensityGrid::GetActiveVoxels() const {
    std::vector<Voxel> activeVoxels;
    
    for (int x = 0; x < m_resolution; ++x) {
        for (int y = 0; y < m_resolution; ++y) {
            for (int z = 0; z < m_resolution; ++z) {
                int index = GetVoxelIndex(x, y, z);
                float density = m_gridData[index];
                
                if (density > 0.0f) {
                    // Visual position center of the voxel
                    Vector3 center(
                        -m_bound + x * m_voxelSize + m_voxelSize / 2.0,
                        -m_bound + y * m_voxelSize + m_voxelSize / 2.0,
                        -m_bound + z * m_voxelSize + m_voxelSize / 2.0
                    );

                    // Skip voxels inside the Earth core
                    if (center.length() < 1.08) continue;

                    Voxel voxel;
                    voxel.center = center;
                    voxel.density = density;

                    // Color coloring map: high density = red, medium = orange, low = cyan
                    if (density > 8.0f) {
                        voxel.color[0] = 0.95f; voxel.color[1] = 0.25f; voxel.color[2] = 0.25f;
                    } else if (density > 4.0f) {
                        voxel.color[0] = 0.95f; voxel.color[1] = 0.60f; voxel.color[2] = 0.15f;
                    } else {
                        voxel.color[0] = 0.14f; voxel.color[1] = 0.65f; voxel.color[2] = 0.93f;
                    }

                    activeVoxels.push_back(voxel);
                }
            }
        }
    }
    return activeVoxels;
}
