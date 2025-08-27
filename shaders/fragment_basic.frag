#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
    float time;
    vec2 mousePos;
    int animationMode;
} ubo;

void main() {
    vec3 normal = normalize(fragNormal);
    
    // Simple lighting
    vec3 lightPos = vec3(5.0, 5.0, 5.0);
    vec3 lightDir = normalize(lightPos - fragWorldPos);
    float diff = max(dot(normal, lightDir), 0.0);
    
    vec3 ambient = 0.3 * fragColor;
    vec3 diffuse = diff * fragColor;
    
    // Add some shimmer based on time
    float shimmer = 0.5 + 0.5 * sin(ubo.time * 3.0 + fragWorldPos.x + fragWorldPos.y);
    
    vec3 finalColor = ambient + diffuse * shimmer;
    outColor = vec4(finalColor, 1.0);
}