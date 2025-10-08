# Library containing the cpp objects used throughout the repo

set( LIB_NAME "planesMatchingLibs" )

set( SRC_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/match_clusters_libs.cpp
)

if( USE_STATIC_LINKS )
  add_library( ${LIB_NAME} STATIC ${SRC_FILES} )
else()
  add_library( ${LIB_NAME} SHARED ${SRC_FILES} )
endif()


target_include_directories(
    ${LIB_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/objects
)


target_link_libraries(
    ${LIB_NAME} PUBLIC
    clusteringLibs
    ${ROOT_LIBRARIES}
)

install( TARGETS ${LIB_NAME} DESTINATION lib )
