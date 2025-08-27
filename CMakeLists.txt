cmake_minimum_required(VERSION 3.10)
project(FynixEngine)

set(CMAKE_CXX_STANDARD 17)

# Set output exe directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(BULLET_ROOT "${CMAKE_SOURCE_DIR}/extern/bullet3-master")
set(BULLET_INCLUDE_DIR "${BULLET_ROOT}/src")
set(BULLET_LIB_DIR "${BULLET_ROOT}/build/lib")

# Include paths
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/imgui
    ${CMAKE_SOURCE_DIR}/include/imgui/backend
    ${BULLET_INCLUDE_DIR}
)

# Link directories
link_directories(${CMAKE_SOURCE_DIR}/lib)

# Source files
file(GLOB_RECURSE SRC_FILES "${CMAKE_SOURCE_DIR}/src/*.cpp")
list(APPEND SRC_FILES "${CMAKE_SOURCE_DIR}/src/glad.c")

# Executable
add_executable(fynix ${SRC_FILES})

# ✅ Copy exe to root dir after build
add_custom_command(TARGET fynix POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:fynix>
        ${CMAKE_SOURCE_DIR}/fynix.exe
)

# Link libraries
target_link_libraries(fynix
    assimpdll
    imgui
    glfw3
    opengl32
    gdi32
    user32
    kernel32
    libBulletDynamics.a
    libBulletCollision.a
    libLinearMath.a
)
