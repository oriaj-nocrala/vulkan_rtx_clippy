// Shadow Miss Shader - Para ray-traced shadows

#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 2) rayPayloadInEXT vec3 shadowValue;

void main() {
    // El rayo no golpeó nada, por lo tanto no hay sombra
    shadowValue = vec3(1.0); // Sin oclusión
}