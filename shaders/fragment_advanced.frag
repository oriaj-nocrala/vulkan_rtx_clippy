// Advanced Fragment Shader with full RTX effects - Based on HTML clippyrtx.html

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

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 fragColor;
layout(location = 4) in vec3 worldPos;
layout(location = 5) in vec3 viewPos;
layout(location = 6) in vec3 reflectDir;
layout(location = 7) in vec3 viewDirection;
layout(location = 8) in mat3 TBN;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;

// Advanced PBR functions from HTML version
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
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

// Enhanced environment mapping from HTML
vec3 getEnvironmentColor(vec3 direction) {
    vec3 color = vec3(0.0);
    
    // Dynamic sky gradient
    float y = direction.y * 0.5 + 0.5;
    vec3 skyColorTop = vec3(0.1, 0.15, 0.3);
    vec3 skyColorHorizon = vec3(0.3, 0.4, 0.6);
    vec3 skyColorBottom = vec3(0.05, 0.05, 0.1);
    
    vec3 skyColor;
    if (y > 0.5) {
        skyColor = mix(skyColorHorizon, skyColorTop, (y - 0.5) * 2.0);
    } else {
        skyColor = mix(skyColorBottom, skyColorHorizon, y * 2.0);
    }
    
    // Simulate ground reflection with grid
    if(direction.y < 0.0) {
        float groundIntensity = abs(direction.y);
        skyColor = mix(skyColor, vec3(0.02, 0.02, 0.04), groundIntensity);
        
        // Animated grid pattern
        float gridX = step(0.98, abs(sin(direction.x * 20.0 + ubo.time)));
        float gridZ = step(0.98, abs(sin(direction.z * 20.0 + ubo.time)));
        float grid = max(gridX, gridZ);
        skyColor += vec3(0.0, 0.5, 0.5) * grid * 0.3 * groundIntensity;
    }
    
    // Animated light sources in environment
    vec3 light1Dir = normalize(vec3(sin(ubo.time), cos(ubo.time), 1.0));
    vec3 light2Dir = normalize(vec3(-cos(ubo.time * 0.8), sin(ubo.time * 0.8), 1.0));
    vec3 light3Dir = normalize(vec3(sin(ubo.time * 1.2), 0.7, cos(ubo.time * 1.2)));
    
    float light1 = pow(max(0.0, dot(direction, light1Dir)), 50.0);
    float light2 = pow(max(0.0, dot(direction, light2Dir)), 50.0);
    float light3 = pow(max(0.0, dot(direction, light3Dir)), 30.0);
    
    color = skyColor;
    color += vec3(1.0, 0.8, 0.3) * light1 * 2.0;
    color += vec3(0.3, 0.8, 1.0) * light2 * 1.5;
    color += vec3(0.8, 0.3, 1.0) * light3 * 1.0;
    
    // Stars/particles in reflection
    float stars = smoothstep(0.99, 1.0, sin(direction.x * 100.0) * sin(direction.y * 100.0) * sin(direction.z * 100.0));
    color += vec3(stars) * 0.8;
    
    return color;
}

// Screen space reflections approximation
vec3 getSSR(vec3 viewPos, vec3 normal) {
    vec2 screenUV = gl_FragCoord.xy / ubo.resolution;
    vec3 reflectDirection = reflect(normalize(viewPos), normal);
    
    // March in screen space
    vec2 marchUV = screenUV;
    vec3 color = vec3(0.0);
    
    for(int i = 0; i < 10; i++) {
        marchUV += reflectDirection.xy * 0.01;
        
        // Bounds check
        if(marchUV.x < 0.0 || marchUV.x > 1.0 || marchUV.y < 0.0 || marchUV.y > 1.0) break;
        
        // Sample from virtual framebuffer (simulated)
        float pattern = sin(marchUV.x * 50.0 + ubo.time) * cos(marchUV.y * 50.0 - ubo.time);
        float distance = length(marchUV - screenUV);
        float falloff = 1.0 / (1.0 + distance * 10.0);
        
        color += vec3(0.1, 0.3, 0.5) * pattern * 0.1 * falloff;
    }
    
    return color;
}

// Multi-layer reflection system from HTML
vec3 getLayeredReflection(vec3 viewDir, vec3 normal, float rough) {
    vec3 reflection = vec3(0.0);
    
    // Primary reflection
    vec3 R1 = reflect(-viewDir, normal);
    reflection += getEnvironmentColor(R1) * (1.0 - rough);
    
    // Secondary reflection with temporal perturbation
    vec3 R2 = reflect(-viewDir, normal + vec3(sin(ubo.time * 2.0) * 0.1, 0.0, cos(ubo.time * 2.0) * 0.1));
    reflection += getEnvironmentColor(R2) * 0.4 * (1.0 - rough * 0.5);
    
    // Blurred reflection for roughness simulation
    if(rough > 0.01) {
        for(float i = 0.0; i < 6.0; i++) {
            float angle = i * 1.047; // 60 degrees
            vec3 offset = vec3(cos(angle), sin(angle), cos(angle * 0.5)) * rough * 0.3;
            vec3 R3 = reflect(-viewDir, normalize(normal + offset));
            reflection += getEnvironmentColor(R3) * 0.15;
        }
    }
    
    return reflection;
}

// Global illumination approximation
vec3 getGlobalIllumination(vec3 normal, vec3 worldPos) {
    vec3 gi = vec3(0.0);
    
    // Sample hemisphere around normal
    for(float phi = 0.0; phi < 6.28; phi += 1.57) {
        for(float theta = 0.0; theta < 1.57; theta += 0.52) {
            vec3 sampleDir = vec3(
                sin(theta) * cos(phi),
                cos(theta),
                sin(theta) * sin(phi)
            );
            
            // Transform to world space
            sampleDir = normalize(mix(sampleDir, normal, 0.6));
            
            // Sample environment
            vec3 envColor = getEnvironmentColor(sampleDir);
            
            // Weight by cosine
            float weight = max(0.0, dot(sampleDir, normal));
            gi += envColor * weight * 0.1;
        }
    }
    
    return gi;
}

// Enhanced caustics with proper wave simulation
float getCaustics(vec3 worldPos) {
    float caustics = 0.0;
    vec3 causticPos = worldPos * 3.0;
    
    // Multiple octaves of caustic patterns
    for(float i = 1.0; i <= 4.0; i++) {
        float freq = i * 2.0;
        float amp = 1.0 / i;
        caustics += sin(causticPos.x * freq - ubo.time * (3.0 + i)) * 
                   cos(causticPos.z * freq + ubo.time * (2.0 + i * 0.5)) * amp;
    }
    
    // Normalize and enhance
    caustics = caustics * 0.5 + 0.5;
    caustics = pow(caustics, 2.0);
    
    return caustics;
}

// Volumetric scattering simulation
vec3 getVolumetricScattering(vec3 rayStart, vec3 rayDir, float steps) {
    vec3 scattering = vec3(0.0);
    vec3 stepSize = rayDir / steps;
    vec3 currentPos = rayStart;
    
    for(float i = 0.0; i < steps; i++) {
        currentPos += stepSize;
        
        // Distance to light sources
        vec3 lightPos1 = vec3(sin(ubo.time) * 5.0, 5.0, cos(ubo.time) * 5.0);
        vec3 lightPos2 = vec3(-cos(ubo.time * 0.8) * 4.0, 3.0, -sin(ubo.time * 0.8) * 4.0);
        
        float dist1 = length(currentPos - lightPos1);
        float dist2 = length(currentPos - lightPos2);
        
        // Scattering contribution
        float scatter1 = 1.0 / (1.0 + dist1 * dist1);
        float scatter2 = 1.0 / (1.0 + dist2 * dist2);
        
        scattering += vec3(1.0, 0.8, 0.3) * scatter1 * 0.01;
        scattering += vec3(0.3, 0.8, 1.0) * scatter2 * 0.01;
    }
    
    return scattering;
}

void main() {
    vec3 normal = normalize(fragNormal);
    vec3 viewDir = normalize(ubo.cameraPos - worldPos);
    
    // Base material properties
    vec3 baseAlbedo = fragColor;
    float metallic = ubo.metallic;
    float roughness = ubo.roughness;
    float ao = 1.0;
    
    // Enhanced procedural patterns
    float pattern1 = sin(fragTexCoord.x * 20.0 + ubo.time) * cos(fragTexCoord.y * 20.0 - ubo.time) * 0.1;
    float pattern2 = sin(fragTexCoord.x * 40.0 - ubo.time * 2.0) * 0.05;
    float pattern3 = cos(length(fragTexCoord - 0.5) * 30.0 + ubo.time * 3.0) * 0.03;
    
    baseAlbedo += vec3(pattern1 * 0.3, pattern2 * 0.2, pattern3 * 0.1);
    
    // Holographic effect with multiple layers
    float hologram1 = sin(worldPos.y * 30.0 - ubo.time * 5.0) * 0.5 + 0.5;
    float hologram2 = sin(worldPos.y * 50.0 - ubo.time * 8.0) * 0.3 + 0.7;
    vec3 hologramColor = vec3(0.0, hologram1 * 0.15, hologram2 * 0.25);
    
    // Mouse interaction with advanced ripples
    float mouseDist = distance(fragTexCoord, ubo.mousePos);
    float ripple1 = sin(mouseDist * 20.0 - ubo.time * 8.0) * 0.5 + 0.5;
    float ripple2 = sin(mouseDist * 35.0 - ubo.time * 12.0) * 0.3 + 0.7;
    float mouseGlow = smoothstep(0.5, 0.0, mouseDist) * (ripple1 * 0.4 + ripple2 * 0.2);
    
    // PBR setup
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, baseAlbedo, metallic);
    
    // Enhanced lighting calculation
    vec3 Lo = vec3(0.0);
    
    // Multiple animated light sources
    vec3 lightPositions[4];
    lightPositions[0] = vec3(sin(ubo.time * 0.5) * 6.0, 6.0, cos(ubo.time * 0.5) * 6.0);
    lightPositions[1] = vec3(-cos(ubo.time * 0.7) * 4.0, 4.0, -sin(ubo.time * 0.7) * 4.0);
    lightPositions[2] = vec3(sin(ubo.time * 1.2) * 3.0, 2.0, cos(ubo.time * 1.2) * 3.0);
    lightPositions[3] = vec3(0.0, -2.0 + sin(ubo.time * 2.0) * 1.0, 0.0);
    
    vec3 lightColors[4];
    lightColors[0] = vec3(1.0, 0.9, 0.7) * 8.0;
    lightColors[1] = vec3(0.3, 0.8, 1.0) * 6.0;
    lightColors[2] = vec3(1.0, 0.3, 0.8) * 4.0;
    lightColors[3] = vec3(0.8, 1.0, 0.3) * 3.0;
    
    // Calculate lighting for each light source
    for(int i = 0; i < 4; ++i) {
        vec3 L = normalize(lightPositions[i] - worldPos);
        vec3 H = normalize(viewDir + L);
        float distance = length(lightPositions[i] - worldPos);
        float attenuation = 1.0 / (1.0 + distance * distance * 0.1);
        vec3 radiance = lightColors[i] * attenuation;
        
        // PBR BRDF
        float NDF = DistributionGGX(normal, H, roughness);
        float G = GeometrySmith(normal, viewDir, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, viewDir), 0.0), F0);
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;
        
        float NdotL = max(dot(normal, L), 0.0);
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * NdotL + 0.0001;
        vec3 specular = numerator / denominator;
        
        Lo += (kD * baseAlbedo / PI + specular) * radiance * NdotL;
    }
    
    // RTX Enhanced Effects
    vec3 rtxColor = vec3(0.0);
    if(ubo.rtxEnabled > 0) {
        // Multi-layered reflections
        vec3 reflection = getLayeredReflection(viewDir, normal, roughness);
        
        // Screen space reflections
        vec3 ssr = getSSR(viewPos, normal);
        
        // Global illumination
        vec3 gi = getGlobalIllumination(normal, worldPos);
        
        // Enhanced caustics
        float caustics = getCaustics(worldPos);
        vec3 causticColor = vec3(0.2, 0.6, 1.0) * caustics * (1.0 - roughness) * 0.3;
        
        // Volumetric scattering
        vec3 volumetric = getVolumetricScattering(worldPos, viewDir, 8.0);
        
        // Refraction for glass-like effect
        vec3 refractDir = refract(-viewDir, normal, 1.0 / 1.5);
        vec3 refraction = getEnvironmentColor(refractDir) * 0.4;
        
        // Chromatic aberration
        float aberration = 0.003 * (1.0 - roughness);
        vec3 chromaticR = getEnvironmentColor(refractDir + vec3(aberration, 0.0, 0.0));
        vec3 chromaticB = getEnvironmentColor(refractDir - vec3(aberration, 0.0, 0.0));
        
        // Fresnel for blending
        vec3 F = fresnelSchlickRoughness(max(dot(normal, viewDir), 0.0), F0, roughness);
        
        // Combine all RTX effects
        rtxColor = reflection * F * 0.8 + 
                  refraction * (1.0 - F) * 0.3 + 
                  gi * 0.2 * (1.0 - metallic) +
                  ssr * 0.15 +
                  causticColor +
                  volumetric +
                  vec3(chromaticR.r * 0.03, 0.0, chromaticB.b * 0.03);
    }
    
    // Ambient lighting
    vec3 ambient = vec3(0.05, 0.08, 0.12) * baseAlbedo * ao;
    
    // Combine all effects
    vec3 finalColor = baseAlbedo * (1.0 - float(ubo.rtxEnabled) * 0.6) + 
                     rtxColor * float(ubo.rtxEnabled) * 0.8 +
                     ambient + 
                     Lo +
                     hologramColor +
                     vec3(mouseGlow) * 0.4;
    
    // Enhanced emissive glow
    vec3 emission = F0 * ubo.glowIntensity * vec3(1.0, 0.8, 0.3) * 
                   (0.6 + 0.4 * sin(ubo.time * 4.0)) * metallic;
    finalColor += emission;
    
    // Advanced tone mapping (ACES)
    const float A = 2.51;
    const float B = 0.03;
    const float C = 2.43;
    const float D = 0.59;
    const float E = 0.14;
    
    finalColor = (finalColor * (A * finalColor + B)) / (finalColor * (C * finalColor + D) + E);
    
    // Gamma correction
    finalColor = pow(finalColor, vec3(1.0/2.2));
    
    // Output with alpha for transparency effects
    float alpha = 0.95 + 0.05 * sin(ubo.time * 2.0);
    outColor = vec4(finalColor, alpha);
}