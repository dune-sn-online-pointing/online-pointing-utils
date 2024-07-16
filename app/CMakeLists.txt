# create the executables for the apps

cmessage( STATUS "Creating cluster_to_root app..." )
add_executable( cluster_to_root ${CMAKE_CURRENT_SOURCE_DIR}/cluster_to_root.cpp )
target_link_libraries( cluster_to_root utils-libraries )
install( TARGETS cluster_to_root DESTINATION bin )

cmessage( STATUS "Creating superimpose_signal_and_backgrounds.cpp app..." )
add_executable( superimpose_signal_and_backgrounds ${CMAKE_CURRENT_SOURCE_DIR}/superimpose_signal_and_backgrounds.cpp )
target_link_libraries( superimpose_signal_and_backgrounds utils-libraries )
install( TARGETS superimpose_signal_and_backgrounds DESTINATION bin )

cmessage( STATUS "Creating match_clusters.cpp app..." )
add_executable( match_clusters ${CMAKE_CURRENT_SOURCE_DIR}/match_clusters.cpp )
target_link_libraries( match_clusters utils-libraries )
install( TARGETS match_clusters DESTINATION bin )

cmessage( STATUS "Creating create_volume_clusters.cpp app..." )
add_executable( create_volume_clusters ${CMAKE_CURRENT_SOURCE_DIR}/create_volume_clusters.cpp )
target_link_libraries( create_volume_clusters utils-libraries )
install( TARGETS create_volume_clusters DESTINATION bin )

