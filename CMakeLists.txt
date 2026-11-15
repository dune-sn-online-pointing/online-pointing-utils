# modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-15 at 18:13:33 UTC
cmake_minimum_required(VERSION 3.17)

project(online-pointing-utils)

# Use C++20 to match conda ROOT 6.40 (root-config reports --has-cxx20 yes).
# ROOT 6.40 headers (e.g. TClass.h) require C++17+; compiling them under C++14
# with libc++ breaks on std::atomic copy semantics. dependencies.cmake refines
# this from root-config when it is on PATH.
set(CMAKE_CXX_STANDARD 20)
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
add_subdirectory( ${CMAKE_SOURCE_DIR}/src/ana )
add_subdirectory( ${CMAKE_SOURCE_DIR}/src/app )

if( WITH_ROOT )
  add_subdirectory( ${CMAKE_SOURCE_DIR}/src/inputs )
endif()

################################################################
# Link python libraries?

# add_subdirectory(${CMAKE_SOURCE_DIR}/python)
