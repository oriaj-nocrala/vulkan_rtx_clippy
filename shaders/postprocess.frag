#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D colorTexture;
layout(binding = 1) uniform sampler2D bloomTexture;

layout(binding = 2) uniform PostProcessUniforms {
    float time;
    float exposure;
    float gamma;
    float contrast;
    float saturation;
    float vignetteStrength;
    float chromaticAberration;
    float filmGrain;
    float bloomIntensity;
    float bloomRadius;
    vec2 resolution;
    int enableTonemap;
    int enableBloom;
    int enableVignette;
    int enableChromaticAberration;
    int enableFilmGrain;
} ubo;

// ACES tone mapping
vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

// Film grain effect
float rand(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

// Vignette effect
float vignette(vec2 uv, float intensity) {
    vec2 center = uv - 0.5;
    float dist = length(center);
    return 1.0 - smoothstep(0.3, 1.0, dist * intensity);
}

// Chromatic aberration
vec3 chromaticAberration(sampler2D tex, vec2 uv, float amount) {
    vec2 offset = (uv - 0.5) * amount;
    float r = texture(tex, uv + offset).r;
    float g = texture(tex, uv).g;
    float b = texture(tex, uv - offset).b;
    return vec3(r, g, b);
}

// Color grading functions
vec3 adjustContrast(vec3 color, float contrast) {
    return ((color - 0.5) * contrast) + 0.5;
}

vec3 adjustSaturation(vec3 color, float saturation) {
    float luminance = dot(color, vec3(0.299, 0.587, 0.114));
    return mix(vec3(luminance), color, saturation);
}

// Advanced bloom with multiple taps
vec3 getBloom(vec2 uv) {
    vec3 bloom = vec3(0.0);
    float totalWeight = 0.0;
    
    // Multiple tap bloom for better quality
    for (int x = -2; x <= 2; x++) {
        for (int y = -2; y <= 2; y++) {
            vec2 offset = vec2(float(x), float(y)) * ubo.bloomRadius / ubo.resolution;
            float weight = exp(-float(x*x + y*y) * 0.5);
            bloom += texture(bloomTexture, uv + offset).rgb * weight;
            totalWeight += weight;
        }
    }
    
    return bloom / totalWeight;
}

// Temporal dithering for film grain
float temporalDither(vec2 uv) {
    return fract(rand(uv + fract(ubo.time)) + rand(uv + fract(ubo.time * 1.618)) * 0.618);
}

void main() {
    vec2 uv = fragTexCoord;
    vec3 color;
    
    // Sample base color with optional chromatic aberration
    if (ubo.enableChromaticAberration == 1) {
        color = chromaticAberration(colorTexture, uv, ubo.chromaticAberration);
    } else {
        color = texture(colorTexture, uv).rgb;
    }
    
    // Add bloom
    if (ubo.enableBloom == 1) {
        vec3 bloomColor = getBloom(uv);
        color += bloomColor * ubo.bloomIntensity;
    }
    
    // Tone mapping
    if (ubo.enableTonemap == 1) {
        color *= ubo.exposure;
        color = ACESFilm(color);
    }
    
    // Gamma correction
    color = pow(color, vec3(1.0 / ubo.gamma));
    
    // Color grading
    color = adjustContrast(color, ubo.contrast);
    color = adjustSaturation(color, ubo.saturation);
    
    // Vignette
    if (ubo.enableVignette == 1) {
        float vignetteAmount = vignette(uv, ubo.vignetteStrength);
        color *= vignetteAmount;
    }
    
    // Film grain
    if (ubo.enableFilmGrain == 1) {
        float grain = temporalDither(uv * ubo.resolution) * 2.0 - 1.0;
        color += grain * ubo.filmGrain;
    }
    
    // Final clamp and output
    color = clamp(color, 0.0, 1.0);
    outColor = vec4(color, 1.0);
}