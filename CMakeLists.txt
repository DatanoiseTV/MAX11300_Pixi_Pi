cmake_minimum_required(VERSION 3.10)
project(pixi)
set(CMAKE_C_STANDARD 11)#C11
set(CMAKE_CXX_STANDARD 17)#C17
set(CMAKE_CXX_STANDARD_REQUIRED ON)

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR})

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin) 
SET(CMAKE_CXX_FLAGS "-O3")

find_package(SDL2 REQUIRED SDL2)
include_directories(${SDL2_INCLUDE_DIRS})
add_executable(main spi.cpp Pixi.cpp)
target_link_libraries(main PRIVATE SDL2 bcm2835 lo)
add_custom_target (run COMMAND ${EXECUTABLE_OUTPUT_PATH}/main)
