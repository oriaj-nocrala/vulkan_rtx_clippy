// Closest Hit Shader - Shading cuando el rayo golpea geometría

#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_ray_tracing_position_fetch : require

// Ray payload structure with recursion depth
struct RayPayload {
    vec3 color;
    int depth;
    float distance;
};

layout(location = 0) rayPayloadInEXT RayPayload payload;
layout(location = 1) rayPayloadEXT vec3 reflectionValue;
layout(location = 2) rayPayloadEXT vec3 shadowValue;

hitAttributeEXT vec2 attribs;

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 3, set = 0) buffer readonly VertexBuffer {
    float vertices[];
};
layout(binding = 4, set = 0) buffer readonly IndexBuffer {
    uint indices[];
};

layout(binding = 2, set = 0) uniform CameraProperties {
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

struct Vertex {
    vec3 pos;
    vec3 normal;
    vec2 texCoord;
    vec3 color;
};

const float PI = 3.14159265359;

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

Vertex getVertex(uint index) {
    uint vertexIndex = indices[index] * 12; // 12 floats per vertex (3+3+2+3+padding)
    
    Vertex v;
    v.pos = vec3(vertices[vertexIndex], vertices[vertexIndex + 1], vertices[vertexIndex + 2]);
    v.normal = vec3(vertices[vertexIndex + 3], vertices[vertexIndex + 4], vertices[vertexIndex + 5]);
    v.texCoord = vec2(vertices[vertexIndex + 6], vertices[vertexIndex + 7]);
    v.color = vec3(vertices[vertexIndex + 8], vertices[vertexIndex + 9], vertices[vertexIndex + 10]);
    
    return v;
}

void main() {
    // Obtener información del triángulo
    uint baseIndex = gl_PrimitiveID * 3;
    
    Vertex v0 = getVertex(baseIndex);
    Vertex v1 = getVertex(baseIndex + 1);
    Vertex v2 = getVertex(baseIndex + 2);
    
    // Interpolación baricéntrica
    vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
    
    // Interpolar atributos
    vec3 worldPos = v0.pos * barycentrics.x + v1.pos * barycentrics.y + v2.pos * barycentrics.z;
    vec3 worldNormal = normalize(v0.normal * barycentrics.x + v1.normal * barycentrics.y + v2.normal * barycentrics.z);
    vec2 texCoord = v0.texCoord * barycentrics.x + v1.texCoord * barycentrics.y + v2.texCoord * barycentrics.z;
    vec3 albedo = v0.color * barycentrics.x + v1.color * barycentrics.y + v2.color * barycentrics.z;
    
    // Transformar al espacio mundial
    worldPos = gl_ObjectToWorldEXT * vec4(worldPos, 1.0);
    worldNormal = normalize(gl_ObjectToWorldEXT * vec4(worldNormal, 0.0));
    
    // Añadir animación procedural al Clippy
    float wave = sin(cam.time * 2.0 + worldPos.y * 3.0) * 0.05;
    worldPos.x += wave;
    worldPos.z += wave * 0.5;
    
    // Material del Clippy - oro metálico
    float metallic = cam.metallic;
    float roughness = cam.roughness;
    float ao = 1.0;
    
    // Patrón procedural para más detalle
    float pattern = sin(texCoord.x * 20.0 + cam.time) * cos(texCoord.y * 20.0 - cam.time) * 0.1;
    albedo += vec3(pattern * 0.2, pattern * 0.1, 0.0);
    
    vec3 N = worldNormal;
    vec3 V = normalize(cam.cameraPos - worldPos);
    
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    
    // Iluminación directa
    vec3 Lo = vec3(0.0);
    
    // Múltiples luces
    vec3 lightPositions[3];
    lightPositions[0] = vec3(sin(cam.time * 0.2) * 5.0, 5.0, cos(cam.time * 0.2) * 5.0);
    lightPositions[1] = vec3(-4.0, 3.0, -4.0);
    lightPositions[2] = vec3(0.0, -3.0, 0.0);
    
    vec3 lightColors[3];
    lightColors[0] = vec3(1.0, 0.9, 0.7) * 8.0;
    lightColors[1] = vec3(0.5, 0.7, 1.0) * 5.0;
    lightColors[2] = vec3(1.0, 0.3, 0.5) * 3.0;
    
    for(int i = 0; i < 3; ++i) {
        vec3 L = normalize(lightPositions[i] - worldPos);
        vec3 H = normalize(V + L);
        float distance = length(lightPositions[i] - worldPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lightColors[i] * attenuation;
        
        // Ray-traced shadows
        shadowValue = vec3(1.0);
        float tMin = 0.001;
        float tMax = distance - 0.001;
        
        traceRayEXT(topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT, 
                   0xff, 1, 0, 2, worldPos, tMin, L, tMax, 2);
        
        float shadow = shadowValue.x;
        radiance *= shadow;
        
        // PBR BRDF
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
    
    // Ambient con IBL simulado
    vec3 ambient = vec3(0.05) * albedo * ao;
    
    // Ray-traced reflections
    vec3 R = reflect(-V, N);
    float fresnel = pow(1.0 - max(dot(N, V), 0.0), 2.0);
    
    if (metallic > 0.1 && payload.depth < 2) {
        reflectionValue = vec3(0.0);
        float tMin = 0.001;
        float tMax = 100.0;
        
        traceRayEXT(topLevelAS, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 1, worldPos, tMin, R, tMax, 1);
        
        vec3 reflectedColor = reflectionValue;
        ambient += reflectedColor * fresnel * metallic * (1.0 - roughness);
    }
    
    // Emissive glow para el Clippy
    vec3 emissive = albedo * 0.15 * (0.7 + 0.3 * sin(cam.time * 4.0));
    
    // Efecto holográfico avanzado
    float hologram = sin(worldPos.y * 50.0 - cam.time * 8.0) * 0.5 + 0.5;
    vec3 hologramColor = vec3(0.0, hologram * 0.1, hologram * 0.2) * (1.0 - roughness);
    
    vec3 color = ambient + Lo + emissive + hologramColor;
    
    // Update payload with results
    payload.color = color;
    payload.distance = gl_HitTEXT;
}