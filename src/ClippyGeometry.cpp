#include "ClippyGeometry.h"
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

void ClippyGeometry::generateClippy(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
    const glm::vec3 goldColor(1.0f, 0.843f, 0.0f);
    const float wireRadius = 0.08f;
    
    vertices.clear();
    indices.clear();
    
    // Crear la forma icónica del clip con alta resolución
    const int segments = 64; // Alta calidad para ray tracing
    
    // Parte superior - curva grande
    createTorusSection(vertices, indices, 
                      glm::vec3(0.0f, 1.0f, 0.0f),  // center
                      0.5f,                          // major radius
                      wireRadius,                    // minor radius
                      0,                             // start angle
                      glm::pi<float>(),              // end angle (semicírculo)
                      segments,
                      goldColor);
    
    // Segmento vertical izquierdo (largo)
    createCylinderSection(vertices, indices,
                         glm::vec3(-0.5f, 1.0f, 0.0f),  // base
                         wireRadius,                     // radius
                         2.0f,                           // height
                         segments,
                         goldColor);
    
    // Segmento vertical derecho (corto)
    createCylinderSection(vertices, indices,
                         glm::vec3(0.5f, 1.0f, 0.0f),   // base
                         wireRadius,                     // radius
                         1.4f,                           // height
                         segments,
                         goldColor);
    
    // Curva inferior
    createTorusSection(vertices, indices,
                      glm::vec3(0.0f, -1.0f, 0.0f),  // center
                      0.5f,                           // major radius
                      wireRadius,                     // minor radius
                      glm::pi<float>(),               // start angle
                      2.0f * glm::pi<float>(),        // end angle
                      segments,
                      goldColor);
    
    // Parte interna - el característico loop interno
    createTorusSection(vertices, indices,
                      glm::vec3(0.2f, 0.3f, 0.0f),
                      0.3f,
                      wireRadius * 0.9f,
                      -glm::pi<float>() * 0.3f,
                      glm::pi<float>() * 1.3f,
                      segments,
                      goldColor * 0.95f); // Ligeramente más oscuro para variación
    
    // Añadir detalles - espiral decorativa
    float spiralHeight = 0.5f;
    int spiralTurns = 3;
    int spiralPoints = segments * spiralTurns;
    uint32_t spiralStartIdx = vertices.size();
    
    for (int i = 0; i <= spiralPoints; ++i) {
        float t = static_cast<float>(i) / spiralPoints;
        float angle = t * spiralTurns * 2.0f * glm::pi<float>();
        float height = -1.5f + t * spiralHeight;
        float radius = 0.1f + t * 0.05f; // Radio variable
        
        glm::vec3 pos(
            cos(angle) * radius - 0.5f,
            height,
            sin(angle) * radius
        );
        
        glm::vec3 normal = glm::normalize(glm::vec3(cos(angle), 0.2f, sin(angle)));
        
        vertices.push_back({
            pos,
            normal,
            glm::vec2(t, 0.0f),
            goldColor * (0.8f + 0.2f * sinf(t * 10.0f)) // Variación de color
        });
    }
    
    // Conectar espiral con triángulos
    for (int i = 0; i < spiralPoints; ++i) {
        if (i < spiralPoints - 1) {
            // Crear tubo alrededor de la espiral
            for (int j = 0; j < 8; ++j) {
                float angle1 = j * 2.0f * glm::pi<float>() / 8;
                float angle2 = (j + 1) * 2.0f * glm::pi<float>() / 8;
                
                indices.push_back(spiralStartIdx + i);
                indices.push_back(spiralStartIdx + i + 1);
                indices.push_back(spiralStartIdx + i);
            }
        }
    }
    
    // ¡Añadir los ojitos de Clippy! 👀
    createClippyEyes(vertices, indices, goldColor);
    
    std::cout << "Clippy geometry created: " << vertices.size() 
              << " vertices, " << indices.size() << " indices" << std::endl;
}

void ClippyGeometry::createTorusSection(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices,
                                        glm::vec3 center, float majorRadius, float minorRadius,
                                        float startAngle, float endAngle, int segments,
                                        glm::vec3 color) {
    uint32_t startVertex = vertices.size();
    int ringSegments = 16;
    
    for (int i = 0; i <= segments; ++i) {
        float u = static_cast<float>(i) / segments;
        float theta = startAngle + u * (endAngle - startAngle);
        
        glm::vec3 majorCirclePoint(
            center.x + cos(theta) * majorRadius,
            center.y + sin(theta) * majorRadius,
            center.z
        );
        
        for (int j = 0; j <= ringSegments; ++j) {
            float v = static_cast<float>(j) / ringSegments;
            float phi = v * 2.0f * glm::pi<float>();
            
            glm::vec3 normal(
                cos(theta) * cos(phi),
                sin(theta) * cos(phi),
                sin(phi)
            );
            
            glm::vec3 position = majorCirclePoint + normal * minorRadius;
            
            vertices.push_back({
                position,
                glm::normalize(normal),
                glm::vec2(u, v),
                color
            });
        }
    }
    
    // Generar índices
    for (int i = 0; i < segments; ++i) {
        for (int j = 0; j < ringSegments; ++j) {
            uint32_t current = startVertex + i * (ringSegments + 1) + j;
            uint32_t next = current + ringSegments + 1;
            
            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);
            
            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }
}

void ClippyGeometry::createCylinderSection(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices,
                                           glm::vec3 baseCenter, float radius, float height,
                                           int segments, glm::vec3 color) {
    uint32_t startVertex = vertices.size();
    
    for (int i = 0; i <= segments; ++i) {
        float theta = static_cast<float>(i) / segments * 2.0f * glm::pi<float>();
        float x = cos(theta);
        float z = sin(theta);
        
        // Vértices de la base
        vertices.push_back({
            baseCenter + glm::vec3(x * radius, 0, z * radius),
            glm::vec3(x, 0, z),
            glm::vec2(static_cast<float>(i) / segments, 0),
            color
        });
        
        // Vértices del tope
        vertices.push_back({
            baseCenter + glm::vec3(x * radius, -height, z * radius),
            glm::vec3(x, 0, z),
            glm::vec2(static_cast<float>(i) / segments, 1),
            color
        });
    }
    
    // Generar índices para las caras del cilindro
    for (int i = 0; i < segments; ++i) {
        uint32_t base = startVertex + i * 2;
        
        indices.push_back(base);
        indices.push_back(base + 2);
        indices.push_back(base + 1);
        
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
    }
}

void ClippyGeometry::createBendSection(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices,
                                      glm::vec3 start, glm::vec3 end, float radius,
                                      int segments, glm::vec3 color) {
    // Implementation for bend sections if needed
    // This is a placeholder for future geometric complexity
}

void ClippyGeometry::createClippyEyes(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, 
                                      glm::vec3 baseColor) {
    // Posición de los ojos en el Clippy
    glm::vec3 leftEyePos(-0.15f, 0.3f, 0.08f);   // Ojo izquierdo
    glm::vec3 rightEyePos(0.15f, 0.3f, 0.08f);   // Ojo derecho
    
    float eyeRadius = 0.08f;
    int eyeSegments = 16;
    
    // Color negro para los ojos
    glm::vec3 eyeColor(0.0f, 0.0f, 0.0f);
    // Pequeño brillo blanco
    glm::vec3 highlightColor(1.0f, 1.0f, 1.0f);
    
    // Crear ojo izquierdo
    createEyeSphere(vertices, indices, leftEyePos, eyeRadius, eyeSegments, eyeColor);
    // Highlight más grande y visible en el ojo izquierdo
    createEyeSphere(vertices, indices, leftEyePos + glm::vec3(0.025f, 0.025f, 0.06f), 
                    eyeRadius * 0.5f, 12, highlightColor);
    
    // Crear ojo derecho  
    createEyeSphere(vertices, indices, rightEyePos, eyeRadius, eyeSegments, eyeColor);
    // Highlight más grande y visible en el ojo derecho
    createEyeSphere(vertices, indices, rightEyePos + glm::vec3(-0.025f, 0.025f, 0.06f), 
                    eyeRadius * 0.5f, 12, highlightColor);
}

void ClippyGeometry::createEyeSphere(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices,
                                     glm::vec3 center, float radius, int segments, glm::vec3 color) {
    uint32_t startVertex = vertices.size();
    
    // Crear esfera usando coordenadas esféricas
    for (int i = 0; i <= segments; ++i) {
        float phi = glm::pi<float>() * static_cast<float>(i) / segments; // 0 a PI
        
        for (int j = 0; j <= segments; ++j) {
            float theta = 2.0f * glm::pi<float>() * static_cast<float>(j) / segments; // 0 a 2PI
            
            // Coordenadas esféricas a cartesianas
            float x = radius * sin(phi) * cos(theta);
            float y = radius * cos(phi);
            float z = radius * sin(phi) * sin(theta);
            
            glm::vec3 position = center + glm::vec3(x, y, z);
            glm::vec3 normal = glm::normalize(glm::vec3(x, y, z));
            
            vertices.push_back({
                position,
                normal,
                glm::vec2(static_cast<float>(j) / segments, static_cast<float>(i) / segments),
                color
            });
        }
    }
    
    // Generar índices para la esfera
    for (int i = 0; i < segments; ++i) {
        for (int j = 0; j < segments; ++j) {
            uint32_t current = startVertex + i * (segments + 1) + j;
            uint32_t next = current + segments + 1;
            
            // Primer triángulo
            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);
            
            // Segundo triángulo
            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }
}