cmake_minimum_required(VERSION 3.16)
project(ClippyRTX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include FetchContent for automatic dependency downloads
include(FetchContent)

# Find system packages
find_package(Vulkan REQUIRED)
find_package(Threads REQUIRED)

# Download and configure GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.3.8
)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# Download and configure GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        0.9.9.8
)

FetchContent_MakeAvailable(glm)

# Opciones de compilación
option(ENABLE_VALIDATION_LAYERS "Enable Vulkan validation layers" ON)
option(ENABLE_RAY_TRACING_DEBUG "Enable extensive ray tracing debugging" ON)

# Archivos fuente
set(SOURCES
    src/main.cpp
    src/VulkanHelpers.cpp
    src/ClippyGeometry.cpp
    src/ClippyRTXApp.cpp
    src/RayTracingPipeline.cpp
    src/VulkanPipelineImplementations.cpp
    src/ClippyUI.cpp
    src/PostProcessing.cpp
)

set(HEADERS
    include/VulkanHelpers.h
    include/ClippyGeometry.h
    include/RayTracingPipeline.h
    include/ClippyRTXApp.h
    include/Vertex.h
    include/ClippyUI.h
    include/PostProcessing.h
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
    glm
    ${CMAKE_THREAD_LIBS_INIT}
)

# Definir macros
if(ENABLE_VALIDATION_LAYERS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_VALIDATION_LAYERS)
endif()

if(ENABLE_RAY_TRACING_DEBUG)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_RAY_TRACING_DEBUG)
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

# Crear directorio de shaders compilados
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)

# Copiar shaders al directorio de build
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/shaders"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
)

# Compilar shaders GLSL a SPIR-V
find_program(GLSLC glslc HINTS ${Vulkan_GLSLC_EXECUTABLE})
find_program(GLSLANG_VALIDATOR glslangValidator HINTS ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE})

if(GLSLC)
    message(STATUS "Found glslc: ${GLSLC}")
    set(SHADER_COMPILER ${GLSLC})
    set(SHADER_COMPILER_FLAGS "")
elseif(GLSLANG_VALIDATOR)
    message(STATUS "Found glslangValidator: ${GLSLANG_VALIDATOR}")
    set(SHADER_COMPILER ${GLSLANG_VALIDATOR})
    set(SHADER_COMPILER_FLAGS "-V;--target-env;spirv1.5")
else()
    message(WARNING "No GLSL compiler found - shaders will not be compiled automatically")
    message(STATUS "Please ensure Vulkan SDK is installed or install vulkan-tools package")
endif()

if(SHADER_COMPILER)
    # Compile all shaders including RTX
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
        set(SPIRV "${CMAKE_BINARY_DIR}/shaders/${FILE_NAME}.spv")
        add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/shaders/"
            COMMAND ${SHADER_COMPILER} ${SHADER_COMPILER_FLAGS} ${GLSL} -o ${SPIRV}
            DEPENDS ${GLSL}
            COMMENT "Compiling shader: ${FILE_NAME}"
        )
        list(APPEND SPIRV_BINARY_FILES ${SPIRV})
    endforeach(GLSL)

    add_custom_target(
        Shaders 
        DEPENDS ${SPIRV_BINARY_FILES}
    )

    add_dependencies(${PROJECT_NAME} Shaders)
endif()

# Información del build
message(STATUS "")
message(STATUS "Clippy RTX - Vulkan Ray Tracing")
message(STATUS "================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Vulkan found: ${Vulkan_FOUND}")
message(STATUS "Vulkan include: ${Vulkan_INCLUDE_DIRS}")
message(STATUS "Vulkan libraries: ${Vulkan_LIBRARIES}")
message(STATUS "GLFW found: ${glfw3_FOUND}")
message(STATUS "GLM found: ${glm_FOUND}")
message(STATUS "Validation layers: ${ENABLE_VALIDATION_LAYERS}")
message(STATUS "")