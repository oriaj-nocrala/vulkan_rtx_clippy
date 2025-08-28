// Closest Hit Shader - Shading cuando el rayo golpea geometría

#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_ray_tracing_position_fetch : require

layout(location = 0) rayPayloadInEXT vec3 hitValue;
layout(location = 1) rayPayloadEXT vec3 reflectionValue;
layout(location = 2) rayPayloadEXT vec3 shadowValue;

hitAttributeEXT vec2 attribs;

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;

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
    vec2 mousePos;
    vec2 resolution;
    float glowIntensity;
    int frameCount;
    int maxBounces;
    int samplesPerPixel;
    int isBGRFormat;
} cam;

struct Vertex {
    vec3 pos;
    vec3 normal;
    vec2 texCoord;
    vec3 color;
};

const float PI = 3.14159265359;

// Random number generation for GI sampling
uint rngState;

uint wang_hash(uint seed) {
    seed = (seed ^ 61u) ^ (seed >> 16u);
    seed *= 9u;
    seed = seed ^ (seed >> 4u);
    seed *= 0x27d4eb2du;
    seed = seed ^ (seed >> 15u);
    return seed;
}

float rnd() {
    rngState = wang_hash(rngState);
    return float(rngState) / 4294967296.0;
}

vec2 rnd2() {
    return vec2(rnd(), rnd());
}

// Cosine-weighted hemisphere sampling for diffuse GI
vec3 cosineWeightedSample(vec3 normal) {
    vec2 r = rnd2();
    float phi = 2.0 * PI * r.x;
    float cosTheta = sqrt(r.y);
    float sinTheta = sqrt(1.0 - r.y);
    
    // Build orthonormal basis around normal
    vec3 w = normal;
    vec3 u = normalize(cross((abs(w.x) > 0.1 ? vec3(0, 1, 0) : vec3(1, 0, 0)), w));
    vec3 v = cross(w, u);
    
    return normalize(u * cos(phi) * sinTheta + v * sin(phi) * sinTheta + w * cosTheta);
}

// Aliases for consistent naming
float random() {
    return rnd();
}

vec3 cosineSampleHemisphere(vec3 normal) {
    return cosineWeightedSample(normal);
}

// PBR functions
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;
    
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

void main() {
    // 🥇 STEP 1: SIMPLE GOLD CLIPPY - No complex PBR, just basic color
    
    // Simple gold color - bright and visible
    vec3 goldColor = vec3(1.0, 0.8, 0.2); // Base gold
    
    // Add simple lighting with world position
    vec3 worldPos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    vec3 rayDir = normalize(gl_WorldRayDirectionEXT);
    
    // Simple fake normal - just opposite of ray direction
    vec3 simpleNormal = normalize(-rayDir);
    
    // Simple directional light
    vec3 lightDir = normalize(vec3(0.3, 1.0, 0.2));
    float lightingFactor = max(0.2, dot(simpleNormal, lightDir)); // Never fully dark
    
    // Apply lighting to gold
    vec3 litGold = goldColor * lightingFactor;
    
    // Ensure minimum brightness - never black
    litGold = max(litGold, vec3(0.3, 0.2, 0.05));
    
    hitValue = litGold;
}