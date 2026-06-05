#ifndef SHADERS_H
#define SHADERS_H

// OpenGL GLSL Shader Sources
namespace Shaders {

    // Earth Sphere Shader
    const char* earthVertexShader = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aTexCoords;

        out vec3 FragPos;
        out vec3 Normal;
        out vec2 TexCoords;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        void main() {
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal;
            TexCoords = aTexCoords;
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";

    const char* earthFragmentShader = R"(
        #version 330 core
        out vec4 FragColor;

        in vec3 FragPos;
        in vec3 Normal;
        in vec2 TexCoords;

        uniform vec3 lightDir;
        uniform vec3 viewPos;
        uniform sampler2D earthTexture;
        uniform sampler2D nightLightsTexture;

        void main() {
            // Ambient
            vec3 ambient = 0.15 * vec3(1.0);
            
            // Diffuse Lighting (Sun)
            vec3 norm = normalize(Normal);
            vec3 light = normalize(-lightDir);
            float diff = max(dot(norm, light), 0.0);
            vec3 diffuse = diff * vec3(1.0);

            // Textures mapping
            vec3 dayColor = vec3(0.05, 0.15, 0.35); // Base oceans blue
            vec3 nightColor = vec3(0.96, 0.62, 0.04) * 0.8; // Emissive yellow lights
            
            // Stylized high-tech mixing based on sun shadow boundary
            vec3 result = mix(nightColor, dayColor + diffuse, diff);
            
            FragColor = vec4(result, 1.0);
        }
    )";

    // Particle / Debris Point Shader
    const char* particleVertexShader = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aColor;
        layout (location = 2) in float aSize;

        out vec3 ParticleColor;

        uniform mat4 view;
        uniform mat4 projection;

        void main() {
            ParticleColor = aColor;
            vec4 viewPos = view * vec4(aPos, 1.0);
            gl_Position = projection * viewPos;
            
            // Attenuate point size by distance to camera
            gl_PointSize = aSize * (300.0 / -viewPos.z);
        }
    )";

    const char* particleFragmentShader = R"(
        #version 330 core
        out vec4 FragColor;

        in vec3 ParticleColor;

        void main() {
            // Draw soft round particle points using coord circles
            vec2 circCoords = gl_PointCoord - vec2(0.5);
            if (dot(circCoords, circCoords) > 0.25) {
                discard;
            }
            
            // Edge smoothing radial falloff
            float alpha = 1.0 - double(dot(circCoords, circCoords)) * 4.0;
            FragColor = vec4(ParticleColor, alpha * 0.95);
        }
    )";

    // Volumetric Risk Voxel Shader
    const char* voxelVertexShader = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aColor;

        out vec3 VoxelColor;

        uniform mat4 view;
        uniform mat4 projection;
        uniform float voxelScale;

        void main() {
            VoxelColor = aColor;
            vec4 viewPos = view * vec4(aPos, 1.0);
            gl_Position = projection * viewPos;
            
            // Large size for voxel volumetric glowing boxes
            gl_PointSize = voxelScale * (400.0 / -viewPos.z);
        }
    )";

    const char* voxelFragmentShader = R"(
        #version 330 core
        out vec4 FragColor;

        in vec3 VoxelColor;

        void main() {
            // Volumetric glowing dots
            vec2 circCoords = gl_PointCoord - vec2(0.5);
            float distSq = dot(circCoords, circCoords);
            if (distSq > 0.25) {
                discard;
            }
            
            float intensity = cos(sqrt(distSq) * 3.14159); // Cosine falloff
            FragColor = vec4(VoxelColor, intensity * 0.55);
        }
    )";
}

#endif // SHADERS_H
