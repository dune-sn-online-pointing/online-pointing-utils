cmake_minimum_required(VERSION 3.17)

project(online-pointing-utils)

option( WITH_ROOT "Build also binaries depending on ROOT" ON )

include( ${CMAKE_SOURCE_DIR}/cmake/cmessage.cmake )
include( ${CMAKE_SOURCE_DIR}/cmake/dependencies.cmake )
include( ${CMAKE_SOURCE_DIR}/cmake/submodules.cmake )

# anothe cmake list in app
add_subdirectory( ${CMAKE_SOURCE_DIR}/app )
# add_subdirectory( ${CMAKE_SOURCE_DIR}/lib )

################################################################
# Link C++ libraries

# for the libraries, that are in src and inc.
# We could move src and inc under lib/, 
# then these lines have to go in a separate file there and 
# the line above has to be uncommented
set( LIB_NAME "utils-libraries" )

set( SRC_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/cluster.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/position_calculator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/cluster_to_root_libs.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/superimpose_root_files_libs.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/aggregate_clusters_within_volume_libs.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/match_clusters_libs.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/create_volume_clusters_libs.cpp
)

add_library( ${LIB_NAME} SHARED ${SRC_FILES} )

target_include_directories(
    ${LIB_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)


# if ever this has to be linked to something else
target_link_libraries(
    ${LIB_NAME} PUBLIC
    ${ROOT_LIBRARIES}
)

install( TARGETS ${LIB_NAME} DESTINATION lib )

################################################################
# Link python libraries

# add_subdirectory(${CMAKE_SOURCE_DIR}/python)
