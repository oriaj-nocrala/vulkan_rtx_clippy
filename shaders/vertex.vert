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
    int frameCount;
    int maxBounces;
    int samplesPerPixel;
    int isBGRFormat;
    float volumetricDensity;
    float volumetricScattering;
    float glassRefractionIndex;
    float causticsStrength;
    float subsurfaceScattering;
    float subsurfaceRadius;
    // 🎭 PERSONALITY SYSTEM PARAMETERS
    int personalityMode;        // 0=IDLE, 1=EXCITED, 2=QUANTUM, 3=PARTY, 4=HELPING, 5=THINKING
    float animationStrength;    // Animation intensity multiplier
    vec3 personalityColorA;     // Dynamic color A for personality modes
    vec3 personalityColorB;     // Dynamic color B for personality modes
    float holographicStrength;  // Holographic scan effect intensity
    float glitchIntensity;      // Quantum glitch effect strength
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

// 🎭 PERSONALITY-BASED VERTEX ANIMATION FUNCTIONS

vec3 getPersonalityAnimation(vec3 pos, vec3 normal) {
    vec3 animatedPos = pos;
    
    switch (ubo.personalityMode) {
        case 0: // IDLE - gentle floating
            {
                float wave1 = sin(ubo.time * 1.0 + pos.y * 2.0) * 0.03;
                float wave2 = sin(ubo.time * 0.7 + pos.x * 1.5) * 0.02;
                animatedPos.x += wave1 * ubo.animationStrength;
                animatedPos.y += wave2 * ubo.animationStrength;
            }
            break;
            
        case 1: // EXCITED - rapid bouncing
            {
                float bounce = abs(sin(ubo.time * 8.0 * ubo.animationStrength)) * 0.15;
                float wiggle = sin(ubo.time * 12.0 + pos.x * 10.0) * 0.08;
                animatedPos.y += bounce * ubo.animationStrength;
                animatedPos.x += wiggle * ubo.animationStrength;
                animatedPos.z += sin(ubo.time * 15.0 + pos.y * 5.0) * 0.05 * ubo.animationStrength;
            }
            break;
            
        case 2: // QUANTUM - glitchy erratic movement
            {
                // Base quantum oscillation
                float quantum1 = sin(ubo.time * 20.0 + pos.x * 8.0) * 0.06;
                float quantum2 = cos(ubo.time * 15.0 + pos.y * 6.0) * 0.04;
                float quantum3 = sin(ubo.time * 25.0 + pos.z * 10.0) * 0.08;
                
                animatedPos += vec3(quantum1, quantum2, quantum3) * ubo.animationStrength;
                
                // Glitch displacement - sudden jumps
                float glitchPhase = floor(ubo.time * 10.0) / 10.0;
                vec3 glitchOffset = vec3(
                    sin(glitchPhase * 37.0 + pos.x * 50.0),
                    cos(glitchPhase * 41.0 + pos.y * 60.0),
                    sin(glitchPhase * 43.0 + pos.z * 70.0)
                ) * 0.03 * ubo.glitchIntensity * ubo.animationStrength;
                
                animatedPos += glitchOffset;
            }
            break;
            
        case 3: // PARTY - wild dancing motion
            {
                float partyTime = ubo.time * 6.0 * ubo.animationStrength;
                float dance1 = sin(partyTime + pos.y * 8.0) * 0.12;
                float dance2 = cos(partyTime * 1.3 + pos.x * 6.0) * 0.10;
                float dance3 = sin(partyTime * 0.8 + pos.z * 4.0) * 0.08;
                
                // Scaling effect for party mode
                float scale = 1.0 + sin(partyTime * 2.0) * 0.05 * ubo.animationStrength;
                animatedPos *= scale;
                
                animatedPos += vec3(dance1, dance2, dance3) * ubo.animationStrength;
            }
            break;
            
        case 4: // HELPING - gentle swaying assistance gesture
            {
                float sway1 = sin(ubo.time * 2.5 + pos.x * 1.0) * 0.06;
                float sway2 = cos(ubo.time * 2.0 + pos.z * 1.5) * 0.04;
                float nod = sin(ubo.time * 3.0) * 0.05;
                
                animatedPos.x += sway1 * ubo.animationStrength;
                animatedPos.z += sway2 * ubo.animationStrength;
                animatedPos.y += nod * ubo.animationStrength;
            }
            break;
            
        case 5: // THINKING - slow contemplative rotation
            {
                float think1 = sin(ubo.time * 0.8 + pos.y * 3.0) * 0.04;
                float think2 = cos(ubo.time * 0.6 + pos.x * 2.0) * 0.03;
                
                // Subtle head tilt effect
                animatedPos.x += think1 * ubo.animationStrength;
                animatedPos.z += think2 * ubo.animationStrength;
            }
            break;
    }
    
    // Mouse interaction displacement (universal for all modes)
    vec2 mouseInfluence = inTexCoord - ubo.mousePos;
    float mouseDist = length(mouseInfluence);
    float mouseEffect = smoothstep(0.5, 0.0, mouseDist);
    animatedPos += normal * mouseEffect * 0.1 * sin(ubo.time * 10.0) * ubo.animationStrength;
    
    return animatedPos;
}

void main() {
    // 🎭 PERSONALITY-BASED ANIMATION SYSTEM
    vec3 animatedPos = getPersonalityAnimation(inPosition, inNormal);
    
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