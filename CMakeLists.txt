cmake_minimum_required(VERSION 3.2)
project(OpenGL_projet)

set(PROJECT_NAME AVGL)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -Wall")

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

file(GLOB_RECURSE SRC_FILES src/*.cpp src/*.hpp)

add_subdirectory(lib/glfw)
include_directories(${OPENGL_INCLUDE_DIR} lib/glfw/include lib/glm lib/glm/glc lib/glm/gtx lib/imgui lib/stb )

file(GLOB IMGUI_SRC lib/imgui/*.cpp)
add_library(IMGUI_LIBRARY STATIC ${IMGUI_SRC})

file(GLOB STB_SRC lib/stb/*.c)
add_library(STB_LIBRARY STATIC ${STB_SRC})

set(MAIN_FILES main.cpp)
add_executable(${PROJECT_NAME} ${MAIN_FILES} ${SRC_FILES})

target_link_libraries(${PROJECT_NAME} glfw ${GLFW_LIBRARIES} ${OPENGL_LIBRARIES} ${GLEW_LIBRARY} IMGUI_LIBRARY STB_LIBRARY)
