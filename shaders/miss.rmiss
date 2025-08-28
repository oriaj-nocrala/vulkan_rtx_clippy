// Miss Shader - Cuando el rayo no golpea geometría

#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadInEXT vec3 hitValue;

layout(binding = 3, set = 0) uniform CameraProperties {
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
} cam;

void main() {
    // 🌅 STEP 1: SIMPLE SKY - Basic gradient, no complex calculations
    
    vec3 rayDir = normalize(gl_WorldRayDirectionEXT);
    
    // Simple sky gradient
    float skyFactor = (rayDir.y + 1.0) * 0.5; // 0 to 1 based on Y direction
    
    vec3 horizonColor = vec3(0.6, 0.8, 1.0); // Light blue
    vec3 zenithColor = vec3(0.2, 0.4, 0.8);  // Darker blue
    
    vec3 skyColor = mix(zenithColor, horizonColor, skyFactor);
    
    // Ensure minimum brightness
    skyColor = max(skyColor, vec3(0.1, 0.15, 0.3));
    
    hitValue = skyColor;
}