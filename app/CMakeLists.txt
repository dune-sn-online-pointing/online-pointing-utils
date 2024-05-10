# create the executables for the apps

cmessage( STATUS "Creating cluster_to_root app..." )
add_executable( cluster_to_root ${CMAKE_CURRENT_SOURCE_DIR}/cluster_to_root.cpp )
target_link_libraries( cluster_to_root utils-libraries )
install( TARGETS cluster_to_root DESTINATION bin )

cmessage( STATUS "Creating superimpose_root_files.cpp app..." )
add_executable( superimpose_root_files ${CMAKE_CURRENT_SOURCE_DIR}/superimpose_root_files.cpp )
target_link_libraries( superimpose_root_files utils-libraries )
install( TARGETS superimpose_root_files DESTINATION bin )

cmessage( STATUS "Creating aggregate_clusters_within_volume.cpp app..." )
add_executable( aggregate_clusters_within_volume ${CMAKE_CURRENT_SOURCE_DIR}/aggregate_clusters_within_volume.cpp )
target_link_libraries( aggregate_clusters_within_volume utils-libraries )
install( TARGETS aggregate_clusters_within_volume DESTINATION bin )

cmessage( STATUS "Creating match_clusters.cpp app..." )
add_executable( match_clusters ${CMAKE_CURRENT_SOURCE_DIR}/match_clusters.cpp )
target_link_libraries( match_clusters utils-libraries )
install( TARGETS match_clusters DESTINATION bin )

cmessage( STATUS "Creating create_volume_clusters.cpp app..." )
add_executable( create_volume_clusters ${CMAKE_CURRENT_SOURCE_DIR}/create_volume_clusters.cpp )
target_link_libraries( create_volume_clusters utils-libraries )
install( TARGETS create_volume_clusters DESTINATION bin )

