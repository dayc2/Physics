cmake_minimum_required(VERSION 3.24)
project(Physics)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

set(SOURCE_FILES main.cpp Solver.hpp ThreadPool.hpp CollisionGrid.hpp Grid.hpp renderer.hpp WindowContextHandler.hpp viewport_handler.hpp)
add_executable(Physics ${SOURCE_FILES})
include_directories(/usr/local/include)

find_package(SFML 2.5 COMPONENTS system window graphics network audio REQUIRED)
include_directories(${SFML_INCLUDE_DIRS})
target_link_libraries(Physics system sfml-window sfml-graphics sfml-audio sfml-network)

