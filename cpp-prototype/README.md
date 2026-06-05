# AETHERIS // C++ & OpenGL Space Debris Visualizer

This folder contains the native C++ desktop prototype implementation for visualizing orbital space debris and calculating volumetric collision risk maps.

## Features

- **Keplerian Orbital Propagator**: Real-time analytical orbit calculations converting Keplerian orbital elements (semi-major axis, eccentricity, inclination, RAAN, argument of perigee, mean anomaly) to ECI Cartesian coordinates using Newton-Raphson iterations to solve Kepler's Equation.
- **Volumetric Density Mapping**: A 3D spatial voxel grid (resolution 32³) overlay showing debris congestion densities and designating glowing risk zones.
- **Modern OpenGL Core Pipeline**: Shaders written in GLSL compiled on runtime, drawing textured Earth, custom particle glow points, and voxel clusters using VBOs and VAOs.
- **Zero-Dependency Core**: Includes a custom lightweight 3D Matrix4 maths library to guarantee compiling out of the box without requiring external header libraries like GLM.

## Mathematical Model

### 1. Orbit Propagation
The Keplerian orbit model translates elements to an Earth-Centered Inertial (ECI) coordinate frame:
- **Mean Anomaly ($M$)**: $M(t) = M_0 + n(t - t_0)$ where $n = \sqrt{\frac{\mu}{a^3}}$ is the mean motion.
- **Kepler's Equation**: Solved via Newton-Raphson approximation:
  $$E_{k+1} = E_k - \frac{E_k - e \sin E_k - M}{1 - e \cos E_k}$$
- **Orbital Plane Coordinates (PQW)**:
  $$x_{\text{orb}} = a(\cos E - e), \quad y_{\text{orb}} = a\sqrt{1 - e^2}\sin E$$
- **Inertial Coordinates (ECI)**: Calculated using 3D Euler rotation matrices:
  - Argument of perigee ($\omega$)
  - Inclination ($i$)
  - Right ascension of ascending node ($\Omega$)

### 2. Volumetric Density Grid
We divide space into a $32 \times 32 \times 32$ voxel grid bounding box of size $[-3.0, 3.0]$ visual units. At each frame:
1. Particles are projected into voxel cells index $(ix, iy, iz)$.
2. Voxel density is computed as the total count of particles inside the cell.
3. Points representing voxel centers are color-coded (Red/Orange/Cyan) and rendered with additive blending to visualize volumetric collision hotspots.

## Build Requirements

1. **Compiler**: `g++` (MSYS2 or MinGW).
2. **Libraries**: OpenGL3, GLFW3.

### Installing Dependencies (MSYS2)
Run the following in your MSYS2 shell to install GLFW:
```bash
pacman -S mingw-w64-x86_64-glfw
```

## Compilation

You can compile the application in two ways:
1. Run the double-clickable `build.bat` batch script in this folder.
2. Manually run the compilation command in your terminal:
   ```bash
   g++ main.cpp OrbitalDebris.cpp DensityGrid.cpp Renderer.cpp -o space_debris_sim.exe -O3 -Wall -lglfw3 -lgdi32 -lopengl32
   ```

## Controls

- `[G]`: Toggle Volumetric Risk Voxel Grid.
- `[Left Arrow]`: Slow down simulation speed.
- `[Right Arrow]`: Speed up simulation speed.
- `[Escape]`: Quit the application.
