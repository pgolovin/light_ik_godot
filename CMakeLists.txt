cmake_minimum_required(VERSION 3.27)
project(light_ik_godot)

message(STATUS "Build Architecture: ${CMAKE_SYSTEM_PROCESSOR}")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
string(REGEX REPLACE "/W[3|4]" "/w" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX")

set(CMAKE_MSVC_RUNTIME_LIBRARY "$<$<CONFIG:Debug>:MultiThreadedDebugDLL>")

# add sub-project
add_subdirectory(${PROJECT_SOURCE_DIR}/light_ik)
add_subdirectory(${PROJECT_SOURCE_DIR}/godot-cpp)
add_subdirectory(${PROJECT_SOURCE_DIR}/light_ik_plugin)


