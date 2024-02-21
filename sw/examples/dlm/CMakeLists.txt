cmake_minimum_required(VERSION 3.0)
project(base)

# Includes
include_directories(${CMAKE_SOURCE_DIR}/../../include)

# AVX support
set(EN_AVX 1 CACHE STRING "AVX environment.")

# Sources
file(GLOB COYOTE_SOURCES ${CMAKE_SOURCE_DIR}/../../src/*.cpp)

# Compilation
set (CMAKE_CXX_STANDARD 17)
if(EN_AVX)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -mavx -march=native -O3")
else()
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -march=native -O1")
endif()

# Boost lib
find_package(Boost COMPONENTS program_options REQUIRED)

# Targets
add_executable(main ${COYOTE_SOURCES} main.cpp)
target_link_libraries(main ${Boost_LIBRARIES})

add_executable(single_node ${COYOTE_SOURCES} single_node.cpp)
target_link_libraries(single_node ${Boost_LIBRARIES})

add_executable(task_gen ${COYOTE_SOURCES} task_gen.cpp)
target_link_libraries(task_gen ${Boost_LIBRARIES})

add_executable(tpcc_gen ${COYOTE_SOURCES} tpcc_gen.cpp)