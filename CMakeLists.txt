cmake_minimum_required(VERSION 3.17)

project(online-pointing-utils)

# Use C++14 standard for better compatibility
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option( USE_STATIC_LINKS "Use static links for generated libraries" ON )
option( WITH_ROOT "Build also binaries depending on ROOT" ON )

include( ${CMAKE_SOURCE_DIR}/cmake/cmessage.cmake )
include( ${CMAKE_SOURCE_DIR}/cmake/dependencies.cmake )
include( ${CMAKE_SOURCE_DIR}/cmake/submodules.cmake )

add_subdirectory( ${CMAKE_SOURCE_DIR}/src/lib )
add_subdirectory( ${CMAKE_SOURCE_DIR}/src/objects )
add_subdirectory( ${CMAKE_SOURCE_DIR}/src/backtracking )
add_subdirectory( ${CMAKE_SOURCE_DIR}/src/clusters )
add_subdirectory( ${CMAKE_SOURCE_DIR}/src/planes-matching )
add_subdirectory( ${CMAKE_SOURCE_DIR}/src/ana )
add_subdirectory( ${CMAKE_SOURCE_DIR}/src/app )

################################################################
# Link python libraries?

# add_subdirectory(${CMAKE_SOURCE_DIR}/python)
