// Miss Shader - Cuando el rayo no golpea geometría

#version 460
#extension GL_EXT_ray_tracing : require

// 🚀 MATCH CLOSEST HIT PAYLOAD STRUCTURE
struct RayPayload {
    vec3 color;          // Final color result
    int depth;           // Current recursion depth  
    int rayType;         // 0=primary, 1=reflection, 2=shadow, 3=GI
    vec3 throughput;     // Energy throughput for path tracing
    uint flags;          // Debug and control flags
};

layout(location = 0) rayPayloadInEXT RayPayload payload;
layout(location = 1) rayPayloadInEXT RayPayload reflectionPayload;
layout(location = 2) rayPayloadInEXT RayPayload shadowPayload;
layout(location = 3) rayPayloadInEXT RayPayload giPayload;

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
    // 🌅 PROFESSIONAL SKY WITH MULTI-PAYLOAD STRUCTURE HANDLING
    
    vec3 rayDir = normalize(gl_WorldRayDirectionEXT);
    
    // Advanced procedural sky
    float skyFactor = (rayDir.y + 1.0) * 0.5;
    
    vec3 horizonColor = vec3(0.8, 0.9, 1.0); // Bright horizon
    vec3 zenithColor = vec3(0.2, 0.4, 0.8);  // Deep blue zenith
    
    vec3 skyColor = mix(zenithColor, horizonColor, skyFactor * skyFactor);
    
    // Add sun disk
    vec3 sunDir = normalize(vec3(0.3, 1.0, 0.2));
    float sunDot = dot(rayDir, sunDir);
    if (sunDot > 0.98) {
        skyColor += vec3(3.0, 2.8, 2.0) * (sunDot - 0.98) * 50.0; // Sun disk
    }
    
    // Atmospheric glow
    float glow = max(0.0, sunDot);
    skyColor += vec3(1.0, 0.8, 0.4) * pow(glow, 4.0) * 0.3;
    
    // Ensure minimum brightness
    skyColor = max(skyColor, vec3(0.15, 0.2, 0.35));
    
    // Set payload color (handles primary, reflection, shadow, and GI rays)
    payload.color = skyColor;
    
    // For shadow rays: no occlusion when hitting sky (full light)
    shadowPayload.color = vec3(1.0); // No shadow from sky
    
    // For GI rays: provide sky lighting contribution
    giPayload.color = skyColor * 0.5; // Reduced sky contribution for GI
}