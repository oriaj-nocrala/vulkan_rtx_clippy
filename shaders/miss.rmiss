// Miss Shader - Cuando el rayo no golpea geometría

#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadInEXT vec3 hitValue;

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

void main() {
    // Crear un skybox procedural para el ambiente de Clippy
    vec3 rayDirection = gl_WorldRayDirectionEXT;
    
    // Gradiente vertical del cielo
    float t = rayDirection.y * 0.5 + 0.5;
    
    // Colores del cielo - azul claro arriba, más cálido abajo
    vec3 skyColorTop = vec3(0.5, 0.7, 1.0);
    vec3 skyColorHorizon = vec3(1.0, 0.9, 0.7);
    vec3 skyColorBottom = vec3(0.8, 0.8, 0.9);
    
    vec3 skyColor;
    if (t > 0.5) {
        // Parte superior del cielo
        skyColor = mix(skyColorHorizon, skyColorTop, (t - 0.5) * 2.0);
    } else {
        // Parte inferior del cielo
        skyColor = mix(skyColorBottom, skyColorHorizon, t * 2.0);
    }
    
    // Añadir algunas "estrellas" o puntos de luz procedurales
    float starNoise = sin(rayDirection.x * 50.0) * sin(rayDirection.y * 50.0) * sin(rayDirection.z * 50.0);
    if (starNoise > 0.95 && rayDirection.y > 0.0) {
        skyColor += vec3(0.3, 0.3, 0.2);
    }
    
    // Efecto de atmósfera dinámica basada en tiempo
    float atmosphere = sin(cam.time * 0.1) * 0.1 + 0.9;
    skyColor *= atmosphere;
    
    // Sol procedural
    vec3 sunDirection = normalize(vec3(sin(cam.time * 0.05), 0.7, cos(cam.time * 0.05)));
    float sunDot = dot(rayDirection, sunDirection);
    if (sunDot > 0.95) {
        float sunIntensity = smoothstep(0.95, 0.99, sunDot);
        skyColor += vec3(2.0, 1.8, 1.0) * sunIntensity;
    }
    
    // Halo solar
    if (sunDot > 0.8) {
        float haloIntensity = smoothstep(0.8, 0.95, sunDot) * 0.3;
        skyColor += vec3(1.0, 0.8, 0.4) * haloIntensity;
    }
    
    // Ambient light para que los objetos no estén completamente negros
    skyColor += vec3(0.05, 0.05, 0.1);
    
    hitValue = skyColor;
}