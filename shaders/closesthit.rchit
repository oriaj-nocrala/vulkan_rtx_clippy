// Closest Hit Shader - Shading cuando el rayo golpea geometría

#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_ray_tracing_position_fetch : require

// 🚀 PROFESSIONAL RAY PAYLOAD STRUCTURE - COMPLETE STATE TRACKING
struct RayPayload {
    vec3 color;          // Final color result
    int depth;           // Current recursion depth  
    int rayType;         // 0=primary, 1=reflection, 2=shadow, 3=GI
    vec3 throughput;     // Energy throughput for path tracing
    uint flags;          // Debug and control flags
};

layout(location = 0) rayPayloadInEXT RayPayload payload;
layout(location = 1) rayPayloadEXT RayPayload reflectionPayload;
layout(location = 2) rayPayloadEXT RayPayload shadowPayload;
layout(location = 3) rayPayloadEXT RayPayload giPayload;

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
    // 🔬 PROFESSIONAL PAYLOAD-BASED DEPTH TRACKING
    int currentDepth = payload.depth;
    int currentRayType = payload.rayType;
    
    // 🚨 CRITICAL INFINITE LOOP PROTECTION
    if (currentDepth > 10) {
        // EMERGENCY ABORT: Infinite recursion detected
        payload.color = vec3(10.0, 0.0, 0.0); // Bright red error
        return;
    }
    
    // 💡 DEBUG: Visual depth indicators
    vec3 depthDebugColor = vec3(0.0);
    if (currentDepth == 0) depthDebugColor = vec3(0.05, 0.0, 0.0);        // Primary: Red tint
    else if (currentDepth == 1) depthDebugColor = vec3(0.0, 0.05, 0.0);   // Depth 1: Green tint  
    else if (currentDepth == 2) depthDebugColor = vec3(0.0, 0.0, 0.05);   // Depth 2: Blue tint
    else depthDebugColor = vec3(0.05, 0.05, 0.0);                         // Deeper: Yellow tint
    
    // 🔧 REAL RAY TRACING WITH PROPER DEPTH CONTROL
    
    // Get world position and surface info
    vec3 worldPos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    vec3 rayDir = normalize(gl_WorldRayDirectionEXT);
    
    // Simple surface normal - opposite of ray direction with slight variation
    vec3 surfaceNormal = normalize(-rayDir + vec3(sin(worldPos.x * 2.0), cos(worldPos.y * 2.0), sin(worldPos.z * 2.0)) * 0.1);
    
    // PBR Material Properties - BRIGHT VIBRANT GOLD
    vec3 albedo = vec3(1.0, 0.85, 0.2); // Brighter, more vibrant gold
    float metallic = max(cam.metallic, 0.9); // Force high metallic for gold
    float roughness = clamp(cam.roughness * 0.6, 0.05, 0.3); // Smoother for shinier gold
    vec3 F0 = mix(vec3(0.04), albedo, metallic); // Fresnel reflectance
    
    // Lighting vectors
    vec3 lightDir = normalize(vec3(0.3, 1.0, 0.2)); // Sun direction
    vec3 viewDir = normalize(cam.cameraPos - worldPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    
    // PBR Calculations - SAFE VERSIONS with clamping
    float NdotL = max(dot(surfaceNormal, lightDir), 0.0);
    float NdotV = max(dot(surfaceNormal, viewDir), 0.01); // Prevent zero
    float NdotH = max(dot(surfaceNormal, halfwayDir), 0.0);
    float VdotH = max(dot(viewDir, halfwayDir), 0.0);
    
    // Only calculate PBR if we have valid lighting
    vec3 finalColor = vec3(0.0);
    
    if (NdotL > 0.0) {
        // Normal Distribution Function (GGX)
        float alpha = roughness * roughness;
        float alpha2 = alpha * alpha;
        float denom = NdotH * NdotH * (alpha2 - 1.0) + 1.0;
        float NDF = alpha2 / max(PI * denom * denom, 0.0001); // Prevent division by zero
        
        // Geometry Function (Smith)
        float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
        float G1L = NdotL / max(NdotL * (1.0 - k) + k, 0.0001);
        float G1V = NdotV / max(NdotV * (1.0 - k) + k, 0.0001);
        float G = G1L * G1V;
        
        // Fresnel (Schlick approximation)
        vec3 F = F0 + (1.0 - F0) * pow(clamp(1.0 - VdotH, 0.0, 1.0), 5.0);
        
        // Cook-Torrance BRDF
        vec3 numerator = NDF * G * F;
        float denominator = max(4.0 * NdotV * NdotL, 0.0001);
        vec3 specular = numerator / denominator;
        
        // Energy conservation
        vec3 kS = F; // Specular contribution
        vec3 kD = vec3(1.0) - kS; // Diffuse contribution
        kD *= 1.0 - metallic; // Metals have no diffuse
        
        // Combine diffuse and specular
        vec3 lightColor = vec3(3.5, 3.0, 2.5); // BRIGHTER warm sun light for gold
        finalColor = (kD * albedo / PI + specular) * lightColor * NdotL;
    }
    
    // 🚀 PROFESSIONAL RECURSIVE GLOBAL ILLUMINATION WITH PAYLOAD TRACKING
    
    vec3 reflectionContrib = vec3(0.0);
    
    // Only trace reflection rays if we haven't reached max depth
    if (metallic > 0.1 && currentDepth < cam.maxBounces) {
        vec3 reflectDir = reflect(rayDir, surfaceNormal);
        
        // 🔧 PROFESSIONAL PAYLOAD SETUP FOR NEXT RAY
        reflectionPayload.color = vec3(0.0);
        reflectionPayload.depth = currentDepth + 1;      // INCREMENT DEPTH  
        reflectionPayload.rayType = 1;                   // Reflection ray
        reflectionPayload.throughput = payload.throughput * metallic;  // Energy conservation
        reflectionPayload.flags = payload.flags;         // Preserve debug flags
        
        // 🎯 DISPATCH REFLECTION RAY WITH PROPER PAYLOAD
        traceRayEXT(topLevelAS,
                   gl_RayFlagsOpaqueEXT,
                   0xff,           // Cull mask
                   0,              // SBT record offset (same shader)
                   0,              // SBT record stride  
                   0,              // Miss index
                   worldPos + surfaceNormal * 0.001,   // Origin with offset
                   0.001,          // tMin
                   reflectDir,     // Direction
                   100.0,          // tMax
                   1);             // Payload location 1
        
        // 🔬 ADVANCED RESULT VALIDATION
        vec3 reflectionResult = reflectionPayload.color;
        if (!any(isnan(reflectionResult)) && !any(isinf(reflectionResult)) &&
            any(greaterThan(reflectionResult, vec3(0.0))) &&
            all(lessThan(reflectionResult, vec3(10.0)))) {
            
            // PROFESSIONAL ENERGY CONSERVATION
            float depthFalloff = 1.0 / (1.0 + float(currentDepth) * 0.3);
            float fresnel = pow(1.0 - max(0.0, dot(-rayDir, surfaceNormal)), 2.0);
            reflectionContrib = reflectionResult * metallic * fresnel * depthFalloff * 0.4;
            
        } else if (any(isnan(reflectionResult))) {
            // DIAGNOSTIC: NaN detected
            reflectionContrib = vec3(1.0, 0.0, 1.0) * 0.05; // Magenta debug
        } else if (any(isinf(reflectionResult))) {
            // DIAGNOSTIC: Infinity detected  
            reflectionContrib = vec3(1.0, 1.0, 0.0) * 0.05; // Yellow debug
        } else {
            // SAFE PROCEDURAL FALLBACK
            float skyFactor = max(0.0, dot(reflectDir, vec3(0.0, 1.0, 0.0)));
            vec3 proceduralReflection = mix(vec3(0.1, 0.15, 0.3), vec3(0.3, 0.5, 0.8), skyFactor);
            reflectionContrib = proceduralReflection * metallic * 0.15;
        }
    }
    
    // Add GOLDEN ambient light - never fully black
    vec3 ambient = albedo * vec3(0.8, 0.6, 0.2) * 0.4; // Warm golden ambient
    finalColor += ambient;
    
    // Add controlled reflection contribution
    finalColor += reflectionContrib;
    
    // 🌟 PROFESSIONAL SHADOW RAYS FOR REALISTIC LIGHTING
    vec3 shadowContrib = vec3(1.0); // Default no shadow
    
    if (currentDepth < cam.maxBounces - 1) { // Leave room for shadow rays
        // Setup shadow ray payload
        shadowPayload.color = vec3(1.0); // Default: no occlusion
        shadowPayload.depth = currentDepth + 1;
        shadowPayload.rayType = 2; // Shadow ray
        shadowPayload.throughput = vec3(1.0);
        shadowPayload.flags = payload.flags;
        
        // Trace shadow ray towards light
        vec3 shadowDir = lightDir;
        traceRayEXT(topLevelAS,
                   gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT,
                   0xff,
                   0, 0, 1, // Use miss shader index 1 for shadows
                   worldPos + surfaceNormal * 0.001,
                   0.001,
                   shadowDir,
                   50.0, // Shadow ray range
                   2); // Payload location 2
        
        // Shadow factor (0 = full shadow, 1 = no shadow)
        float shadowFactor = shadowPayload.color.x;
        shadowContrib = vec3(mix(0.3, 1.0, shadowFactor)); // Soft shadows
    }
    
    // Apply shadows to lighting
    finalColor *= shadowContrib;
    
    // 🌍 GLOBAL ILLUMINATION - Diffuse indirect lighting
    vec3 giContrib = vec3(0.0);
    
    if (currentDepth < cam.maxBounces - 1 && (1.0 - metallic) > 0.1) { // Diffuse surfaces only
        // Initialize GI ray seeding  
        rngState = wang_hash(uint(worldPos.x * 1000.0) + uint(worldPos.y * 2000.0) + uint(currentDepth * 100.0) + uint(cam.frameCount));
        
        // Sample hemisphere for diffuse GI
        vec3 giDir = cosineWeightedSample(surfaceNormal);
        
        giPayload.color = vec3(0.0);
        giPayload.depth = currentDepth + 1;
        giPayload.rayType = 3; // GI ray
        giPayload.throughput = payload.throughput * (1.0 - metallic);
        giPayload.flags = payload.flags;
        
        traceRayEXT(topLevelAS,
                   gl_RayFlagsOpaqueEXT,
                   0xff,
                   0, 0, 0,
                   worldPos + surfaceNormal * 0.001,
                   0.001,
                   giDir,
                   20.0, // GI ray range
                   3); // Payload location 3
        
        // Energy-conserving GI contribution
        float giStrength = 0.3 * (1.0 - metallic); // Only for diffuse surfaces
        float depthFalloff = 1.0 / (1.0 + float(currentDepth) * 0.5);
        giContrib = giPayload.color * albedo * giStrength * depthFalloff;
    }
    
    finalColor += giContrib;
    
    // Add GOLDEN emissive glow for Clippy magic
    vec3 emissive = albedo * 0.15 * (1.0 + sin(cam.time * 3.0) * 0.3); // Stronger glow
    finalColor += emissive;
    
    // Ensure GOLDEN minimum brightness - safety net
    finalColor = max(finalColor, vec3(0.4, 0.3, 0.1)); // Brighter golden minimum
    
    // 🎨 ADD VISUAL DEBUGGING TINTS
    finalColor += depthDebugColor;
    
    // 🔧 GENTLER TONE MAPPING for brighter gold
    finalColor = finalColor / (finalColor + vec3(1.2)); // Less aggressive tone mapping
    
    // 🎯 OUTPUT TO CORRECT PAYLOAD BASED ON RAY TYPE
    payload.color = finalColor;
    payload.throughput = payload.throughput * (1.0 - metallic * 0.5); // Energy tracking
}