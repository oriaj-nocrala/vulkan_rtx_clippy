// Advanced Vertex Shader with RTX effects - Inspired by HTML version

#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 viewInverse;
    mat4 projInverse;
    vec3 cameraPos;
    float time;
    float metallic;
    float roughness;
    int rtxEnabled;
    vec2 mousePos;
    vec2 resolution;
    float glowIntensity;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inColor;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec3 fragColor;
layout(location = 4) out vec3 worldPos;
layout(location = 5) out vec3 viewPos;
layout(location = 6) out vec3 reflectDir;
layout(location = 7) out vec3 viewDirection;
layout(location = 8) out mat3 TBN;

void main() {
    // Enhanced procedural animation
    vec3 animatedPos = inPosition;
    
    // Multiple wave layers for complex motion
    float wave1 = sin(ubo.time * 2.0 + inPosition.y * 3.0) * 0.05;
    float wave2 = sin(ubo.time * 1.5 + inPosition.x * 2.0) * 0.03;
    float wave3 = cos(ubo.time * 3.0 + inPosition.z * 4.0) * 0.02;
    
    animatedPos.x += wave1 + wave2;
    animatedPos.y += wave3;
    animatedPos.z += wave1 * 0.5 + wave2 * 0.3;
    
    // Mouse interaction displacement
    vec2 mouseInfluence = inTexCoord - ubo.mousePos;
    float mouseDist = length(mouseInfluence);
    float mouseEffect = smoothstep(0.5, 0.0, mouseDist);
    animatedPos += inNormal * mouseEffect * 0.1 * sin(ubo.time * 10.0);
    
    // Transform to world space
    vec4 worldPosition = ubo.model * vec4(animatedPos, 1.0);
    worldPos = worldPosition.xyz;
    
    // Transform to view space
    vec4 viewPosition = ubo.view * worldPosition;
    viewPos = viewPosition.xyz;
    fragPos = viewPos;
    
    // Final clip space position
    gl_Position = ubo.proj * viewPosition;
    
    // Enhanced normal calculation for better lighting
    vec3 worldNormal = normalize(mat3(ubo.model) * inNormal);
    fragNormal = worldNormal;
    
    // Simple tangent calculation for vertex shader
    vec3 N = normalize(worldNormal);
    vec3 T = vec3(1.0, 0.0, 0.0);
    if (abs(N.x) > 0.9) {
        T = vec3(0.0, 1.0, 0.0);
    }
    T = normalize(T - N * dot(T, N));
    vec3 B = cross(N, T);
    TBN = mat3(T, B, N);
    
    // View direction for reflections
    viewDirection = normalize(worldPos - ubo.cameraPos);
    
    // Reflection direction for cubemap/environment mapping
    reflectDir = reflect(viewDirection, worldNormal);
    
    // Pass through texture coordinates and color
    fragTexCoord = inTexCoord;
    fragColor = inColor;
}