// Fragment Shader con PBR para fallback sin RTX

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
} ubo;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 fragColor;
layout(location = 4) in vec3 worldPos;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;

// Funciones PBR
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
    vec3 albedo = fragColor; // Color dorado del Clippy
    float metallic = ubo.metallic;
    float roughness = ubo.roughness;
    float ao = 1.0;
    
    // Añadir patrón procedural
    float pattern = sin(fragTexCoord.x * 20.0 + ubo.time) * cos(fragTexCoord.y * 20.0 - ubo.time) * 0.1;
    albedo += vec3(pattern * 0.2, pattern * 0.1, 0.0);
    
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(ubo.cameraPos - worldPos);
    
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    
    // Iluminación
    vec3 Lo = vec3(0.0);
    
    // Luz principal animada
    vec3 lightPositions[3];
    lightPositions[0] = vec3(sin(ubo.time * 0.2) * 5.0, 5.0, cos(ubo.time * 0.2) * 5.0);
    lightPositions[1] = vec3(-4.0, 3.0, -4.0);
    lightPositions[2] = vec3(0.0, -3.0, 0.0);
    
    vec3 lightColors[3];
    lightColors[0] = vec3(1.0, 0.9, 0.7) * 5.0;
    lightColors[1] = vec3(0.5, 0.7, 1.0) * 3.0;
    lightColors[2] = vec3(1.0, 0.3, 0.5) * 2.0;
    
    for(int i = 0; i < 3; ++i) {
        // Cálculo por luz
        vec3 L = normalize(lightPositions[i] - worldPos);
        vec3 H = normalize(V + L);
        float distance = length(lightPositions[i] - worldPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lightColors[i] * attenuation;
        
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;
        
        float NdotL = max(dot(N, L), 0.0);
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.0001;
        vec3 specular = numerator / denominator;
        
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }
    
    // Ambient con fake IBL
    vec3 ambient = vec3(0.03) * albedo * ao;
    
    // Reflexiones de cubemap simuladas (cuando RTX está off)
    if (ubo.rtxEnabled == 0) {
        vec3 R = reflect(-V, N);
        float mipLevel = roughness * 4.0;
        
        // Simular reflexión del entorno
        float fresnel = pow(1.0 - max(dot(N, V), 0.0), 2.0);
        vec3 envColor = mix(vec3(0.1, 0.15, 0.25), vec3(0.5, 0.7, 1.0), R.y * 0.5 + 0.5);
        ambient += envColor * fresnel * (1.0 - roughness) * metallic;
    }
    
    // Emissive
    vec3 emissive = albedo * 0.1 * (0.5 + 0.5 * sin(ubo.time * 3.0));
    
    // Efecto holográfico
    float hologram = sin(worldPos.y * 30.0 - ubo.time * 5.0) * 0.5 + 0.5;
    vec3 hologramColor = vec3(0.0, hologram * 0.05, hologram * 0.1) * (1.0 - roughness);
    
    vec3 color = ambient + Lo + emissive + hologramColor;
    
    // Tone mapping
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));
    
    outColor = vec4(color, 1.0);
}