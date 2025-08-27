#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
    float time;
    vec2 mousePos;
    int animationMode;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inColor;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragWorldPos;

void main() {
    // Disable wave animation for debugging
    vec3 pos = inPosition;
    // float wave = sin(ubo.time * 2.0 + pos.y * 3.0) * 0.1;
    // pos.x += wave;
    
    vec4 worldPos = ubo.model * vec4(pos, 1.0);
    gl_Position = ubo.proj * ubo.view * worldPos;
    
    fragColor = inColor;
    fragTexCoord = inTexCoord;
    fragNormal = mat3(ubo.model) * inNormal;
    fragWorldPos = worldPos.xyz;
}