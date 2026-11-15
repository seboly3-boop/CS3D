cmake_minimum_required(VERSION 3.20)
project(CS3D LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)

# --- GLFW 3.4 (window + input) ---
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
  URL https://github.com/glfw/glfw/archive/refs/tags/3.4.zip)

# --- GLM 1.0.1 (header-only math) ---
FetchContent_Declare(glm
  URL https://github.com/g-truc/glm/archive/refs/tags/1.0.1.zip)

FetchContent_MakeAvailable(glfw glm)

add_executable(CS3D
  src/main.cpp
  src/Player.cpp
  src/Camera.cpp
  src/Weapon.cpp
  src/Animation.cpp
  src/Recoil.cpp
  src/Physics.cpp
  src/Input.cpp
  src/Renderer.cpp
  src/World.cpp
  src/Effects.cpp
  src/Audio.cpp
  src/GL.cpp
)

target_include_directories(CS3D PRIVATE src)
target_link_libraries(CS3D PRIVATE glfw glm::glm)
target_compile_definitions(CS3D PRIVATE NOMINMAX _CRT_SECURE_NO_WARNINGS)

if(WIN32)
  target_link_libraries(CS3D PRIVATE opengl32 winmm)
else()
  find_package(OpenGL REQUIRED)
  target_link_libraries(CS3D PRIVATE OpenGL::GL)
endif()

if(MSVC)
  target_compile_options(CS3D PRIVATE /W3 /utf-8)
  set_property(TARGET CS3D PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT CS3D)
endif()
