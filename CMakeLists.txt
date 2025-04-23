cmake_minimum_required(VERSION 3.17)

project(online-pointing-utils)

option( USE_STATIC_LINKS "Use static links for generated libraries" ON )
option( WITH_ROOT "Build also binaries depending on ROOT" ON )

include( ${CMAKE_SOURCE_DIR}/cmake/cmessage.cmake )
include( ${CMAKE_SOURCE_DIR}/cmake/dependencies.cmake )
include( ${CMAKE_SOURCE_DIR}/cmake/submodules.cmake )

add_subdirectory( ${CMAKE_SOURCE_DIR}/src/calculate-position )
add_subdirectory( ${CMAKE_SOURCE_DIR}/src/objects )
add_subdirectory( ${CMAKE_SOURCE_DIR}/src/clustering )
add_subdirectory( ${CMAKE_SOURCE_DIR}/src/planes-matching )
add_subdirectory( ${CMAKE_SOURCE_DIR}/src/app )

################################################################
# # Link C++ libraries

# set( LIB_NAME "onlinePointingUtils" )

# add_library( ${LIB_NAME} SHARED ${SRC_FILES} )

# target_include_directories(
#     ${LIB_NAME} PUBLIC
#     ${CMAKE_CURRENT_SOURCE_DIR}/inc
# )


# # if ever this has to be linked to something else
# target_link_libraries(
#     ${LIB_NAME} PUBLIC
#     ${ROOT_LIBRARIES}
# )

# install( TARGETS ${LIB_NAME} DESTINATION lib )

################################################################
# Link python libraries?

# add_subdirectory(${CMAKE_SOURCE_DIR}/python)
