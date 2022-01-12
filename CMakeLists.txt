cmake_minimum_required(VERSION 3.10)

set(PROJECT_NAME HeyVulkan)

project(${PROJECT_NAME})

set(${CMAKE_ECLIPSE_GENERATE_SOURCE_PROJECT} TRUE)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/modules)

set(${COMPILE_FLAGS} "${COMPILE_FLAGS} -g")

set(${CMAKE_CXX_STANDARD} C++17)

find_package(GLM REQUIRED)
message(STATUS "GLM included at ${GLM_INCLUDE_DIR}")
find_package(GLFW3 REQUIRED)
message(STATUS "GLFW included at ${GLFW3_INCLUDE_DIR}")
find_package(ASSIMP REQUIRED)
message(STATUS "ASSIMP included at ${ASSIMP_INCLUDE_DIR}")

set(LIBS ${GLFW3_LIBRARY} xcb Xrandr Xinerama Xi Xxf86vm Xcursor GL dl pthread ${ASSIMP_LIBRARY} vulkan)

add_executable(${PROJECT_NAME} "${CMAKE_SOURCE_DIR}/src/main.cpp"
                                "${CMAKE_SOURCE_DIR}/src/Application.cpp")

target_link_libraries(${PROJECT_NAME} ${LIBS})
