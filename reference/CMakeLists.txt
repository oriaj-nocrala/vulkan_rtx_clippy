# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(ClippyRTX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Encontrar paquetes
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(Threads REQUIRED)

# Opciones de compilación
option(ENABLE_VALIDATION_LAYERS "Enable Vulkan validation layers" ON)

# Archivos fuente
set(SOURCES
    src/main.cpp
    src/VulkanHelpers.cpp
    src/ClippyGeometry.cpp
    src/RayTracingPipeline.cpp
    src/ShaderManager.cpp
)

set(HEADERS
    include/VulkanHelpers.h
    include/ClippyGeometry.h
    include/RayTracingPipeline.h
    include/ShaderManager.h
    include/Vertex.h
)

# Crear ejecutable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Incluir directorios
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Vulkan_INCLUDE_DIRS}
)

# Librerías a enlazar
target_link_libraries(${PROJECT_NAME}
    ${Vulkan_LIBRARIES}
    glfw
    glm::glm
    ${CMAKE_THREAD_LIBS_INIT}
)

# Definir macros
if(ENABLE_VALIDATION_LAYERS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_VALIDATION_LAYERS)
endif()

# Optimizaciones para release
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -O3
        -march=native
        -mtune=native
        -ffast-math
    )
endif()

# Copiar shaders al directorio de build
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/shaders"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
)

# Compilar shaders GLSL a SPIR-V
find_program(GLSLC glslc HINTS ${Vulkan_GLSLC_EXECUTABLE})

file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${CMAKE_SOURCE_DIR}/shaders/*.vert"
    "${CMAKE_SOURCE_DIR}/shaders/*.frag"
    "${CMAKE_SOURCE_DIR}/shaders/*.rgen"
    "${CMAKE_SOURCE_DIR}/shaders/*.rmiss"
    "${CMAKE_SOURCE_DIR}/shaders/*.rchit"
    "${CMAKE_SOURCE_DIR}/shaders/*.comp"
)

foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${CMAKE_SOURCE_DIR}/shaders/compiled/${FILE_NAME}.spv")
    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/shaders/compiled/"
        COMMAND ${GLSLC} ${GLSL} -o ${SPIRV}
        DEPENDS ${GLSL}
    )
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    Shaders 
    DEPENDS ${SPIRV_BINARY_FILES}
)

add_dependencies(${PROJECT_NAME} Shaders)

# ============= build.sh =============
# Script de compilación para Linux/Mac

#!/bin/bash

echo "==================================="
echo "   Clippy RTX - Vulkan Ray Tracing"
echo "==================================="

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Verificar dependencias
echo -e "${YELLOW}Verificando dependencias...${NC}"

# Verificar Vulkan SDK
if ! command -v glslc &> /dev/null; then
    echo -e "${RED}Error: Vulkan SDK no encontrado. Por favor instala Vulkan SDK.${NC}"
    echo "Visita: https://vulkan.lunarg.com/"
    exit 1
fi

# Verificar compilador
if ! command -v g++ &> /dev/null; then
    echo -e "${RED}Error: g++ no encontrado.${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Dependencias verificadas${NC}"

# Crear directorios
echo -e "${YELLOW}Creando estructura de directorios...${NC}"
mkdir -p build
mkdir -p shaders/compiled
mkdir -p bin

# Compilar shaders
echo -e "${YELLOW}Compilando shaders...${NC}"
glslc shaders/raygen.rgen -o shaders/compiled/raygen.rgen.spv
glslc shaders/miss.rmiss -o shaders/compiled/miss.rmiss.spv
glslc shaders/closesthit.rchit -o shaders/compiled/closesthit.rchit.spv
glslc shaders/shadow.rmiss -o shaders/compiled/shadow.rmiss.spv
glslc shaders/vertex.vert -o shaders/compiled/vertex.vert.spv
glslc shaders/fragment.frag -o shaders/compiled/fragment.frag.spv

echo -e "${GREEN}✓ Shaders compilados${NC}"

# Compilar con CMake
echo -e "${YELLOW}Configurando CMake...${NC}"
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release

echo -e "${YELLOW}Compilando proyecto...${NC}"
make -j$(nproc)

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Compilación exitosa${NC}"
    echo -e "${YELLOW}Ejecutando Clippy RTX...${NC}"
    echo ""
    ./ClippyRTX
else
    echo -e "${RED}✗ Error en la compilación${NC}"
    exit 1
fi

# ============= build.bat =============
# Script de compilación para Windows

@echo off
echo ===================================
echo    Clippy RTX - Vulkan Ray Tracing
echo ===================================

:: Verificar Vulkan SDK
where glslc >nul 2>nul
if %errorlevel% neq 0 (
    echo Error: Vulkan SDK no encontrado. Por favor instala Vulkan SDK.
    echo Visita: https://vulkan.lunarg.com/
    pause
    exit /b 1
)

echo Dependencias verificadas

:: Crear directorios
echo Creando estructura de directorios...
if not exist build mkdir build
if not exist shaders\compiled mkdir shaders\compiled
if not exist bin mkdir bin

:: Compilar shaders
echo Compilando shaders...
glslc shaders\raygen.rgen -o shaders\compiled\raygen.rgen.spv
glslc shaders\miss.rmiss -o shaders\compiled\miss.rmiss.spv
glslc shaders\closesthit.rchit -o shaders\compiled\closesthit.rchit.spv
glslc shaders\shadow.rmiss -o shaders\compiled\shadow.rmiss.spv
glslc shaders\vertex.vert -o shaders\compiled\vertex.vert.spv
glslc shaders\fragment.frag -o shaders\compiled\fragment.frag.spv

echo Shaders compilados

:: Compilar con CMake
echo Configurando CMake...
cd build
cmake .. -G "Visual Studio 16 2019" -A x64

echo Compilando proyecto...
cmake --build . --config Release

if %errorlevel% equ 0 (
    echo Compilacion exitosa
    echo Ejecutando Clippy RTX...
    Release\ClippyRTX.exe
) else (
    echo Error en la compilacion
    pause
    exit /b 1
)

# ============= README.md =============
# Clippy RTX - Vulkan Ray Tracing

## 🚀 Descripción
Implementación de Clippy (el asistente de Microsoft Office) con ray tracing real usando Vulkan RTX.

## ✨ Características
- **Ray Tracing Real**: Reflexiones, sombras y iluminación global usando RTX
- **PBR Shading**: Materiales físicamente correctos con metallic/roughness workflow
- **Efectos Avanzados**:
  - Reflexiones ray-traced con múltiples bounces
  - Sombras suaves con ray tracing
  - Global illumination aproximada
  - Anti-aliasing temporal
  - Motion blur
  - Depth of field
  - Efectos holográficos

## 📋 Requisitos
- GPU con soporte RTX (NVIDIA RTX 2060 o superior)
- Vulkan SDK 1.3+
- CMake 3.16+
- C++17 compiler (GCC 7+, Clang 9+, MSVC 2019+)
- GLFW 3.3+
- GLM 0.9.9+

## 🔧 Instalación

### Linux/Mac
```bash
# Instalar dependencias
sudo apt-get install vulkan-sdk libglfw3-dev libglm-dev cmake g++

# Clonar y compilar
git clone https://github.com/tu-usuario/clippy-rtx.git
cd clippy-rtx
chmod +x build.sh
./build.sh
```

### Windows
```batch
# Instalar Vulkan SDK desde https://vulkan.lunarg.com/
# Instalar Visual Studio 2019+
# Ejecutar:
build.bat