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

# Verificar CMake
if ! command -v cmake &> /dev/null; then
    echo -e "${RED}Error: CMake no encontrado.${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Dependencias verificadas${NC}"

# Crear directorios
echo -e "${YELLOW}Creando estructura de directorios...${NC}"
mkdir -p build
mkdir -p shaders/compiled
mkdir -p bin

echo -e "${GREEN}✓ Directorios creados${NC}"

# Compilar con CMake
echo -e "${YELLOW}Configurando CMake...${NC}"
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release

echo -e "${YELLOW}Compilando proyecto...${NC}"
make -j$(nproc)

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Compilación exitosa${NC}"
    echo -e "${YELLOW}Ejecutable creado en: build/ClippyRTX${NC}"
    echo ""
    echo -e "${GREEN}Para ejecutar Clippy RTX:${NC}"
    echo -e "${YELLOW}cd build && ./ClippyRTX${NC}"
    echo ""
    echo -e "${YELLOW}Controles:${NC}"
    echo "  SPACE - Toggle RTX On/Off"
    echo "  ESC   - Salir"
else
    echo -e "${RED}✗ Error en la compilación${NC}"
    exit 1
fi