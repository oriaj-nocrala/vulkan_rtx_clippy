#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "Vertex.h"

class ClippyGeometry {
public:
    static void generateClippy(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
    
private:
    static void createTorusSection(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices,
                                   glm::vec3 center, float majorRadius, float minorRadius,
                                   float startAngle, float endAngle, int segments,
                                   glm::vec3 color);
    
    static void createCylinderSection(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices,
                                      glm::vec3 baseCenter, float radius, float height,
                                      int segments, glm::vec3 color);
    
    static void createBendSection(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices,
                                  glm::vec3 start, glm::vec3 end, float radius,
                                  int segments, glm::vec3 color);
    
    // Funciones para crear los ojitos de Clippy 👀
    static void createClippyEyes(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, 
                                 glm::vec3 baseColor);
    
    static void createEyeSphere(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices,
                                glm::vec3 center, float radius, int segments, glm::vec3 color);
};