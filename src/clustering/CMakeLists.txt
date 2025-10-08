set( LIB_NAME "clusteringLibs" )

set( SRC_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/aggregate_clusters_within_volume_libs.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/create_volume_clusters_libs.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/superimpose_root_files_libs.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cluster_to_root_libs.cpp
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
    ${CMAKE_SOURCE_DIR}/src/backtracking
)


target_link_libraries(
    ${LIB_NAME} PUBLIC
    objectsLibs
    globalLib
    ${ROOT_LIBRARIES}
)

install( TARGETS ${LIB_NAME} DESTINATION lib )
