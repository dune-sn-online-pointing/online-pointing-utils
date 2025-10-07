set( LIB_NAME "clusteringLibs" )

set( SRC_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/aggregate_clusters_within_volume_libs.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/create_volume_clusters_libs.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/superimpose_root_files_libs.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/cluster_to_root_libs.cpp
)

if( USE_STATIC_LINKS )
  add_library( ${LIB_NAME} STATIC ${SRC_FILES} )
else()
  add_library( ${LIB_NAME} SHARED ${SRC_FILES} )
endif()


target_include_directories(
    ${LIB_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_SOURCE_DIR}/src/objects/inc
    ${CMAKE_SOURCE_DIR}/src/backtracking
)


target_link_libraries(
    ${LIB_NAME} PUBLIC
    objectsLibs
    globalLib
    ${ROOT_LIBRARIES}
)

install( TARGETS ${LIB_NAME} DESTINATION lib )
