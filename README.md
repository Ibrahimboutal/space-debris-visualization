# AETHERIS // Space Debris Visualizer & Volumetric Risk Mapping

A dual-prototype system designed to visualize orbital trajectories, track space debris congestion, and map volumetric collision risk hotspots.

This project contains two implementations:
1. **Interactive WebGL Dashboard (Web Application)**: A browser-based interface built with Three.js featuring 2,200+ simulated orbital particles, linked telemetry charts, real-time filters, and a Kessler cascade chain-reaction simulator.
2. **Native C++/OpenGL Core Prototype**: A modular, dependency-light desktop client implementing identical orbital calculations and volumetric voxel grid mapping using GLSL shaders and a custom Matrix4 mathematics module.

---

## Project Structure

```text
space-debris-visualization/
│
├── index.html          # Web dashboard layout and telemetry HUD
├── style.css           # Sci-fi cyberpunk design styles (glassmorphism)
├── app.js              # Three.js simulation logic, propagators, and grids
├── server.js           # Lightweight Node.js local development web server
├── README.md           # This project documentation
│
└── cpp-prototype/      # Native C++ OpenGL desktop app
    ├── main.cpp        # GLFW window setup and main simulation loop
    ├── OrbitalDebris.h # Debris structure & Keplerian orbit header
    ├── OrbitalDebris.cpp
    ├── DensityGrid.h   # Voxel density mapper header
    ├── DensityGrid.cpp
    ├── Shaders.h       # Vert/Frag GLSL shader strings
    ├── Renderer.h      # OpenGL VAO/VBO buffer renderer header
    ├── Renderer.cpp
    ├── build.bat       # Windows MSYS2 g++ compile batch script
    └── README.md       # C++ build instructions and math documentation
```

---

## Key Features

- **Procedural 3D Earth Globe**: Stylized cyberpunk digital mesh grid texturing with glowing night lights and additively blended atmosphere shaders (requires no external textures or assets).
- **Keplerian Orbit Propagation Engine**: Analytical solver updating Mean Motion, solving Kepler's Equation using Newton-Raphson iterations, and transforming coordinates into ECI Cartesian space in real-time.
- **Volumetric Spatial Density Grid**: Subdivides orbital space into a $32 \times 32 \times 32$ voxel grid, calculating local debris densities to render glowing amber/red risk zones.
- **HUD Command Interface**: Cyberpunk-inspired dashboard containing:
  - **Filters**: Sort by orbital regime (LEO, MEO, GEO), categories (Satellites, Rocket bodies, Fragmentations), and origin countries.
  - **Live Linked Charts**: Real-time altitude distribution histogram updating with active debris coordinates.
  - **Telemetry Card**: Selection utility allowing the user to select orbital particles to trace their full paths and view live altitude, speed, and calculated collision risk indices.
- **Kessler Syndrome Collision Simulator**: Spawns 350+ hypervelocity fragmentation particles radiating outwards from the collision coordinates, dispersing into new orbits and triggering a `DANGER` alert.

---

## Mathematical Foundation

### 1. Orbit Propagation Model
The Keplerian orbit model translates elements to an Earth-Centered Inertial (ECI) coordinate frame:
- **Mean Motion ($n$)**: $n = \sqrt{\frac{\mu}{a^3}}$ where $\mu = 398600.44 \text{ km}^3/\text{s}^2$ is Earth's standard gravitational parameter.
- **Kepler's Equation**: Solved via Newton-Raphson approximation:
  $$E_{k+1} = E_k - \frac{E_k - e \sin E_k - M}{1 - e \cos E_k}$$
- **Orbital Plane Coordinates (PQW)**:
  $$x_{\text{orb}} = a(\cos E - e), \quad y_{\text{orb}} = a\sqrt{1 - e^2}\sin E$$
- **Inertial Coordinates (ECI)**: Calculated using 3D Euler rotations:
  - Right ascension of ascending node ($\Omega$)
  - Inclination ($i$)
  - Argument of perigee ($\omega$)

### 2. Volumetric Voxel Density Grid
We divide space into a voxel grid bounding box of size $[-3.0, 3.0]$ visual units. At each frame:
1. Active particle coordinates are projected into voxel cell index $(ix, iy, iz)$.
2. Voxel density is computed as the total count of particles inside the cell.
3. Points representing voxel centers are color-coded (Red/Orange/Cyan) and rendered with additive blending to visualize volumetric collision hotspots.

---

## Getting Started

### 1. Running the Web Application
Make sure you have Node.js installed, then execute:

```bash
# Start the local static file server
node server.js
```

1. Open your browser to [http://localhost:8080/](http://localhost:8080/).
2. Rotate the globe by dragging left-click, zoom using scroll wheel, and pan by dragging right-click.
3. Use the left panel to filter debris or toggle the Volumetric Risk Grid.
4. Click on any orbiting dot to see its trajectory and live telemetry parameters.
5. Click **TRIGGER KESSLER CASCADE** to simulate a chain reaction collision.

### 2. Building the C++ Desktop Application
Navigate to the `cpp-prototype` folder:

```bash
cd cpp-prototype
```

Follow the details in [cpp-prototype/README.md](file:///C:/Users/Ibrah/.gemini/antigravity-ide/scratch/space-debris-visualization/cpp-prototype/README.md) to install dependencies (GLFW3) and build the project using MSYS2 `g++`.
