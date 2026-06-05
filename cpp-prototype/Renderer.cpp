#include "Renderer.h"
#include "Shaders.h"
#include <iostream>
#include <cmath>

// Define OpenGL pointer instantiations
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = nullptr;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray = nullptr;
PFNGLGENBUFFERSPROC glGenBuffers = nullptr;
PFNGLBINDBUFFERPROC glBindBuffer = nullptr;
PFNGLBUFFERDATAPROC glBufferData = nullptr;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = nullptr;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = nullptr;
PFNGLCREATESHADERPROC glCreateShader = nullptr;
PFNGLSHADERSOURCEPROC glShaderSource = nullptr;
PFNGLCOMPILESHADERPROC glCompileShader = nullptr;
PFNGLGETSHADERIVPROC glGetShaderiv = nullptr;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = nullptr;
PFNGLCREATEPROGRAMPROC glCreateProgram = nullptr;
PFNGLATTACHSHADERPROC glAttachShader = nullptr;
PFNGLLINKPROGRAMPROC glLinkProgram = nullptr;
PFNGLGETPROGRAMIVPROC glGetProgramiv = nullptr;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = nullptr;
PFNGLUSEPROGRAMPROC glUseProgram = nullptr;
PFNGLDELETESHADERPROC glDeleteShader = nullptr;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = nullptr;
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv = nullptr;
PFNGLUNIFORM3FVPROC glUniform3fv = nullptr;
PFNGLUNIFORM1FPROC glUniform1f = nullptr;

Renderer::Renderer() 
    : m_earthShader(0), m_particleShader(0), m_voxelShader(0),
      m_earthVAO(0), m_earthVBO(0), m_earthEBO(0), m_earthIndexCount(0),
      m_debrisVAO(0), m_debrisVBO(0),
      m_voxelVAO(0), m_voxelVBO(0),
      m_orbitVAO(0), m_orbitVBO(0) {}

Renderer::~Renderer() {}

bool Renderer::InitializeGLPointers() {
    // Dynamically retrieve core modern OpenGL functions in Windows
    glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)wglGetProcAddress("glGenVertexArrays");
    glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)wglGetProcAddress("glBindVertexArray");
    glGenBuffers = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers");
    glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
    glBufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");
    glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glEnableVertexAttribArray");
    glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)wglGetProcAddress("glVertexAttribPointer");
    
    glCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
    glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
    glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
    glGetShaderiv = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
    glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
    glCreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
    glAttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
    glLinkProgram = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
    glGetProgramiv = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
    glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");
    glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
    glDeleteShader = (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader");

    glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
    glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)wglGetProcAddress("glUniformMatrix4fv");
    glUniform3fv = (PFNGLUNIFORM3FVPROC)wglGetProcAddress("glUniform3fv");
    glUniform1f = (PFNGLUNIFORM1FPROC)wglGetProcAddress("glUniform1f");

    if (!glGenVertexArrays || !glBindVertexArray || !glGenBuffers || !glBindBuffer ||
        !glBufferData || !glEnableVertexAttribArray || !glVertexAttribPointer ||
        !glCreateShader || !glCompileShader || !glUseProgram || !glUniformMatrix4fv) {
        std::cerr << "CRITICAL: Failed to load OpenGL core function pointers." << std::endl;
        return false;
    }
    return true;
}

GLuint Renderer::CompileProgram(const char* vertexSrc, const char* fragmentSrc) {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSrc, nullptr);
    glCompileShader(vertexShader);
    
    GLint success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Vertex Shader Compile Error:\n" << infoLog << std::endl;
        return 0;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSrc, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Fragment Shader Compile Error:\n" << infoLog << std::endl;
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Shader Link Error:\n" << infoLog << std::endl;
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

bool Renderer::SetupShaders() {
    m_earthShader = CompileProgram(Shaders::earthVertexShader, Shaders::earthFragmentShader);
    m_particleShader = CompileProgram(Shaders::particleVertexShader, Shaders::particleFragmentShader);
    m_voxelShader = CompileProgram(Shaders::voxelVertexShader, Shaders::voxelFragmentShader);
    
    return (m_earthShader != 0 && m_particleShader != 0 && m_voxelShader != 0);
}

void Renderer::SetupBuffers() {
    // 1. Procedural generation of Earth UV-Sphere mesh
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    const unsigned int X_SEGMENTS = 40;
    const unsigned int Y_SEGMENTS = 40;
    const float PI = 3.14159265359f;

    for (unsigned int y = 0; y <= Y_SEGMENTS; ++y) {
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x) {
            float xSegment = (float)x / (float)X_SEGMENTS;
            float ySegment = (float)y / (float)Y_SEGMENTS;
            float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
            float yPos = std::cos(ySegment * PI);
            float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

            // Position (x,y,z)
            vertices.push_back(xPos);
            vertices.push_back(yPos);
            vertices.push_back(zPos);
            // Normal (x,y,z)
            vertices.push_back(xPos);
            vertices.push_back(yPos);
            vertices.push_back(zPos);
            // Texture coordinates (u,v)
            vertices.push_back(xSegment);
            vertices.push_back(ySegment);
        }
    }

    for (unsigned int y = 0; y < Y_SEGMENTS; ++y) {
        for (unsigned int x = 0; x < X_SEGMENTS; ++x) {
            indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
            indices.push_back(y * (X_SEGMENTS + 1) + x);
            indices.push_back(y * (X_SEGMENTS + 1) + x + 1);

            indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
            indices.push_back(y * (X_SEGMENTS + 1) + x + 1);
            indices.push_back((y + 1) * (X_SEGMENTS + 1) + x + 1);
        }
    }
    m_earthIndexCount = indices.size();

    glGenVertexArrays(1, &m_earthVAO);
    glGenBuffers(1, &m_earthVBO);
    glGenBuffers(1, &m_earthEBO);

    glBindVertexArray(m_earthVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_earthVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_earthEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Attribute pointers: Position, Normal, TexCoords
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

    // 2. Setup dynamic particles VAO/VBO
    glGenVertexArrays(1, &m_debrisVAO);
    glGenBuffers(1, &m_debrisVBO);
    glBindVertexArray(m_debrisVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_debrisVBO);
    
    // Allocate space for up to 3000 elements dynamically: float size per particle: 3 pos + 3 color + 1 size = 7 floats
    glBufferData(GL_ARRAY_BUFFER, 3000 * 7 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(6 * sizeof(float)));

    // 3. Setup dynamic voxel grid points VAO/VBO
    glGenVertexArrays(1, &m_voxelVAO);
    glGenBuffers(1, &m_voxelVBO);
    glBindVertexArray(m_voxelVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_voxelVBO);
    // Voxel point float size: 3 pos + 3 color = 6 floats
    glBufferData(GL_ARRAY_BUFFER, 32 * 32 * 32 * 6 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    // 4. Setup single Orbit Trace VAO/VBO
    glGenVertexArrays(1, &m_orbitVAO);
    glGenBuffers(1, &m_orbitVBO);
    glBindVertexArray(m_orbitVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_orbitVBO);
    glBufferData(GL_ARRAY_BUFFER, 128 * 3 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glBindVertexArray(0);
}

void Renderer::DrawEarth(const float* model, const float* view, const float* projection, const float* sunDir) {
    glUseProgram(m_earthShader);

    glUniformMatrix4fv(glGetUniformLocation(m_earthShader, "model"), 1, GL_FALSE, model);
    glUniformMatrix4fv(glGetUniformLocation(m_earthShader, "view"), 1, GL_FALSE, view);
    glUniformMatrix4fv(glGetUniformLocation(m_earthShader, "projection"), 1, GL_FALSE, projection);
    glUniform3fv(glGetUniformLocation(m_earthShader, "lightDir"), 1, sunDir);

    glBindVertexArray(m_earthVAO);
    glDrawElements(GL_TRIANGLES, m_earthIndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Renderer::DrawDebris(const std::vector<DebrisObject>& debrisList, const float* view, const float* projection) {
    std::vector<float> particleBuffer;
    const float scale = 1.0f / 6371.0f;

    for (const auto& deb : debrisList) {
        if (!deb.visible) continue;

        // Position scaled
        particleBuffer.push_back(deb.position.x * scale);
        particleBuffer.push_back(deb.position.y * scale);
        particleBuffer.push_back(deb.position.z * scale);

        // Color
        if (deb.category == "sat") {
            particleBuffer.push_back(0.08f); particleBuffer.push_back(0.65f); particleBuffer.push_back(0.93f);
        } else if (deb.category == "rocket") {
            particleBuffer.push_back(0.96f); particleBuffer.push_back(0.62f); particleBuffer.push_back(0.04f);
        } else {
            particleBuffer.push_back(0.93f); particleBuffer.push_back(0.27f); particleBuffer.push_back(0.27f);
        }

        // Size
        float size = (deb.category == "sat") ? 4.5f : (deb.category == "rocket") ? 6.5f : 3.0f;
        particleBuffer.push_back(size);
    }

    if (particleBuffer.empty()) return;

    glBindBuffer(GL_ARRAY_BUFFER, m_debrisVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, particleBuffer.size() * sizeof(float), particleBuffer.data());

    glUseProgram(m_particleShader);
    glUniformMatrix4fv(glGetUniformLocation(m_particleShader, "view"), 1, GL_FALSE, view);
    glUniformMatrix4fv(glGetUniformLocation(m_particleShader, "projection"), 1, GL_FALSE, projection);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive lighting blending
    glDepthMask(GL_FALSE);

    glBindVertexArray(m_debrisVAO);
    glDrawArrays(GL_POINTS, 0, particleBuffer.size() / 7);
    
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
}

void Renderer::DrawRiskZones(const DensityGrid& grid, const float* view, const float* projection) {
    auto activeVoxels = grid.GetActiveVoxels();
    if (activeVoxels.empty()) return;

    std::vector<float> voxelBuffer;
    for (const auto& vox : activeVoxels) {
        voxelBuffer.push_back(vox.center.x);
        voxelBuffer.push_back(vox.center.y);
        voxelBuffer.push_back(vox.center.z);

        voxelBuffer.push_back(vox.color[0]);
        voxelBuffer.push_back(vox.color[1]);
        voxelBuffer.push_back(vox.color[2]);
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_voxelVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, voxelBuffer.size() * sizeof(float), voxelBuffer.data());

    glUseProgram(m_voxelShader);
    glUniformMatrix4fv(glGetUniformLocation(m_voxelShader, "view"), 1, GL_FALSE, view);
    glUniformMatrix4fv(glGetUniformLocation(m_voxelShader, "projection"), 1, GL_FALSE, projection);
    glUniform1f(glGetUniformLocation(m_voxelShader, "voxelScale"), (float)grid.GetVoxelSize() * 8.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Transparent glow blending
    glDepthMask(GL_FALSE);

    glBindVertexArray(m_voxelVAO);
    glDrawArrays(GL_POINTS, 0, voxelBuffer.size() / 6);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
}

void Renderer::DrawOrbitLine(const std::vector<Vector3>& pathPoints, const float* view, const float* projection) {
    if (pathPoints.empty()) return;

    std::vector<float> pointsBuffer;
    const float scale = 1.0f / 6371.0f;
    for (const auto& p : pathPoints) {
        pointsBuffer.push_back(p.x * scale);
        pointsBuffer.push_back(p.y * scale);
        pointsBuffer.push_back(p.z * scale);
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_orbitVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, pointsBuffer.size() * sizeof(float), pointsBuffer.data());

    // Reuse particle shader with line strip binding
    glUseProgram(m_particleShader);
    glUniformMatrix4fv(glGetUniformLocation(m_particleShader, "view"), 1, GL_FALSE, view);
    glUniformMatrix4fv(glGetUniformLocation(m_particleShader, "projection"), 1, GL_FALSE, projection);

    glBindVertexArray(m_orbitVAO);
    glLineWidth(2.0f);
    glDrawArrays(GL_LINE_STRIP, 0, pointsBuffer.size() / 3);
    glBindVertexArray(0);
}
