#include <GLFW/glfw3.h>
#include "OrbitalDebris.h"
#include "DensityGrid.h"
#include "Renderer.h"
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

// Self-contained column-major 4x4 matrix implementation to avoid GLM header dependency
struct Matrix4 {
    float m[16];

    Matrix4() {
        Identity();
    }

    void Identity() {
        for (int i = 0; i < 16; ++i) m[i] = 0.0f;
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }

    static Matrix4 Translate(float x, float y, float z) {
        Matrix4 mat;
        mat.m[12] = x;
        mat.m[13] = y;
        mat.m[14] = z;
        return mat;
    }

    static Matrix4 RotateY(float angleRad) {
        Matrix4 mat;
        float c = std::cos(angleRad);
        float s = std::sin(angleRad);
        mat.m[0] = c;
        mat.m[2] = -s;
        mat.m[8] = s;
        mat.m[10] = c;
        return mat;
    }

    static Matrix4 Perspective(float fovDeg, float aspect, float nearVal, float farVal) {
        Matrix4 mat;
        float f = 1.0f / std::tan(fovDeg * 0.5f * 3.14159265f / 180.0f);
        mat.m[0] = f / aspect;
        mat.m[5] = f;
        mat.m[10] = (farVal + nearVal) / (nearVal - farVal);
        mat.m[11] = -1.0f;
        mat.m[14] = (2.0f * farVal * nearVal) / (nearVal - farVal);
        mat.m[15] = 0.0f;
        return mat;
    }

    static Matrix4 LookAt(float px, float py, float pz, float tx, float ty, float tz) {
        Vector3 eye(px, py, pz);
        Vector3 target(tx, ty, tz);
        
        Vector3 zaxis = Vector3(eye.x - target.x, eye.y - target.y, eye.z - target.z).normalized();
        Vector3 up(0.0f, 1.0f, 0.0f);
        
        // cross product for xaxis
        Vector3 xaxis;
        xaxis.x = up.y * zaxis.z - up.z * zaxis.y;
        xaxis.y = up.z * zaxis.x - up.x * zaxis.z;
        xaxis.z = up.x * zaxis.y - up.y * zaxis.x;
        xaxis = xaxis.normalized();

        // cross product for yaxis
        Vector3 yaxis;
        yaxis.x = zaxis.y * xaxis.z - zaxis.z * xaxis.y;
        yaxis.y = zaxis.z * xaxis.x - zaxis.x * xaxis.z;
        yaxis.z = zaxis.x * xaxis.y - zaxis.y * xaxis.x;

        Matrix4 mat;
        mat.m[0] = xaxis.x;
        mat.m[4] = xaxis.y;
        mat.m[8] = xaxis.z;
        
        mat.m[1] = yaxis.x;
        mat.m[5] = yaxis.y;
        mat.m[9] = yaxis.z;
        
        mat.m[2] = zaxis.x;
        mat.m[6] = zaxis.y;
        mat.m[10] = zaxis.z;

        mat.m[12] = -(xaxis.x * eye.x + xaxis.y * eye.y + xaxis.z * eye.z);
        mat.m[13] = -(yaxis.x * eye.x + yaxis.y * eye.y + yaxis.z * eye.z);
        mat.m[14] = -(zaxis.x * eye.x + zaxis.y * eye.y + zaxis.z * eye.z);
        
        return mat;
    }
};

// Simulation State
double simTime = 0.0;
double timeScale = 100.0;
bool showDensityGrid = true;
std::vector<DebrisObject> debrisList;
float cameraAngle = 0.0f;
float cameraDist = 4.5f;

// Initialize mock orbits
void GenerateMockDebris(int count) {
    std::srand(std::time(nullptr));
    const std::string categories[] = {"sat", "rocket", "frag"};
    
    for (int idx = 0; idx < count; ++idx) {
        DebrisObject obj;
        obj.id = 20000 + idx;
        obj.name = "OBJ-" + std::to_string(obj.id);
        
        // Mix of LEO, MEO, GEO orbits
        double roll = (double)std::rand() / RAND_MAX;
        if (roll < 0.70) {
            obj.a = 6600.0 + (double)std::rand() / RAND_MAX * 1400.0; // LEO
            obj.regime = "leo";
        } else if (roll < 0.90) {
            obj.a = 9000.0 + (double)std::rand() / RAND_MAX * 17000.0; // MEO
            obj.regime = "meo";
        } else {
            obj.a = 41800.0 + (double)std::rand() / RAND_MAX * 600.0; // GEO
            obj.regime = "geo";
        }

        obj.e = ((double)std::rand() / RAND_MAX < 0.15) ? (0.05 + (double)std::rand() / RAND_MAX * 0.4) : ((double)std::rand() / RAND_MAX * 0.01);
        obj.i = (double)std::rand() / RAND_MAX * 3.14159;
        obj.raan = (double)std::rand() / RAND_MAX * 2.0 * 3.14159;
        obj.arg = (double)std::rand() / RAND_MAX * 2.0 * 3.14159;
        obj.ma = (double)std::rand() / RAND_MAX * 2.0 * 3.14159;
        
        // Mean motion: n = sqrt(mu / a^3)
        obj.n = std::sqrt(OrbitPropagator::MU / std::pow(obj.a, 3));
        
        obj.category = categories[std::rand() % 3];
        obj.visible = true;
        obj.risk = 0.0;

        debrisList.push_back(obj);
    }
}

// Window Keyboard callbacks
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        } else if (key == GLFW_KEY_G) {
            showDensityGrid = !showDensityGrid;
            std::cout << "HUD: Density grid volume toggled: " << (showDensityGrid ? "ON" : "OFF") << std::endl;
        } else if (key == GLFW_KEY_RIGHT) {
            timeScale = std::min(timeScale * 2.0, 1000.0);
            std::cout << "HUD: Time acceleration set to: " << timeScale << "x" << std::endl;
        } else if (key == GLFW_KEY_LEFT) {
            timeScale = std::max(timeScale / 2.0, 1.0);
            std::cout << "HUD: Time acceleration set to: " << timeScale << "x" << std::endl;
        }
    }
}

int main() {
    std::cout << "=====================================================" << std::endl;
    std::cout << "AETHERIS // Space Debris Visualizer & Risk Mapping" << std::endl;
    std::cout << "=====================================================" << std::endl;

    if (!glfwInit()) {
        std::cerr << "CRITICAL: Failed to initialize GLFW." << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "AETHERIS - Space Debris Visualizer", nullptr, nullptr);
    if (!window) {
        std::cerr << "CRITICAL: Failed to create GLFW Window." << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSwapInterval(1); // Enable V-sync

    // Initialize Renderer
    Renderer renderer;
    if (!renderer.InitializeGLPointers()) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    if (!renderer.SetupShaders()) {
        std::cerr << "CRITICAL: Failed to compile shaders." << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    renderer.SetupBuffers();

    // Generate mock datasets (1500 debris particles)
    GenerateMockDebris(1500);
    std::cout << "Dataset: Generated " << debrisList.size() << " debris objects using Keplerian orbital elements." << std::endl;

    // Density Voxel Grid: 32x32x32 resolution
    DensityGrid grid(32, 3.0);

    // Render configuration
    glEnable(GL_DEPTH_TEST);
    glPointSize(5.0f);

    double lastTime = glfwGetTime();

    std::cout << "\nHUD CONTROLS:" << std::endl;
    std::cout << "[G] Toggle Risk Volume Grid" << std::endl;
    std::cout << "[Left Arrow] Decrease simulation speed" << std::endl;
    std::cout << "[Right Arrow] Increase simulation speed" << std::endl;
    std::cout << "[Esc] Quit Prototype" << std::endl;
    std::cout << "\nRunning main simulation loop...\n" << std::endl;

    // Simulation Main loop
    while (!glfwWindowShouldClose(window)) {
        double currentRealTime = glfwGetTime();
        double delta = currentRealTime - lastTime;
        lastTime = currentRealTime;

        // Propagate simulation time
        simTime += delta * timeScale;

        // Update Debris Positions in real-time
        for (auto& deb : debrisList) {
            OrbitPropagator::Propagate(deb, simTime);
        }

        // Recalculate Volumetric Density Grid
        grid.Update(debrisList);

        // Get window sizes
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);

        // Clear buffers
        glClearColor(0.012f, 0.027f, 0.047f, 1.0f); // Deep space dark blue
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Camera transformations
        cameraAngle += 0.12f * (float)delta; // Auto-rotate camera
        float camX = cameraDist * std::sin(cameraAngle);
        float camZ = cameraDist * std::cos(cameraAngle);

        Matrix4 projection = Matrix4::Perspective(45.0f, (float)width / (float)height, 0.1f, 100.0f);
        Matrix4 view = Matrix4::LookAt(camX, 2.0f, camZ, 0.0f, 0.0f, 0.0f);
        Matrix4 model; // Identity matrix for Earth orientation

        // Slowly rotate Earth independently
        model = Matrix4::RotateY((float)simTime * 0.005f);

        // Rotating Sun light direction
        float sunAngle = (float)simTime * 0.0001f;
        float sunDir[3] = { std::cos(sunAngle), -0.5f, std::sin(sunAngle) };

        // 1. Draw Earth Globe
        renderer.DrawEarth(model.m, view.m, projection.m, sunDir);

        // 2. Draw Debris particle clouds
        renderer.DrawDebris(debrisList, view.m, projection.m);

        // 3. Draw Volumetric Voxel Collision Risk zones
        if (showDensityGrid) {
            renderer.DrawRiskZones(grid, view.m, projection.m);
        }

        // Swap visual buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
