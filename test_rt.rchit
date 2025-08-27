#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadInEXT vec3 hitValue;

void main() {
    // Test which ray tracing variables are available
    float testValue = 0.0;
    
    // Try different possible names
    // testValue += gl_RayRecursionDepthEXT;     // This failed
    // testValue += gl_RayRecursionDepthKHR;     // Try KHR version
    // testValue += gl_RayRecursionDepthNV;      // Try NV version
    
    // Maybe it's not available, use alternative approach
    // For now, just use builtin variables that work
    testValue += gl_LaunchIDEXT.x * 0.01;
    // testValue += float(gl_RayRecursionDepth);  // Try without EXT
    
    hitValue = vec3(testValue, 0.0, 0.0);
}