cmessage( STATUS "Creating clustering app..." )
add_executable( clustering ${CMAKE_CURRENT_SOURCE_DIR}/src/cluster_to_root.cpp )
target_link_libraries( clustering clusteringLibs )
install( TARGETS clustering DESTINATION bin )


# cmessage( STATUS "Creating createVolume app..." )
# add_executable( createVolume ${CMAKE_CURRENT_SOURCE_DIR}/src/create_volume_clusters.cpp )
# target_link_libraries( createVolume clusteringLibs )
# install( TARGETS createVolume DESTINATION bin )

# cmessage( STATUS "Creating matchClusters app..." )
# add_executable( matchClusters ${CMAKE_CURRENT_SOURCE_DIR}/src/match_clusters.cpp )
# target_link_libraries( matchClusters planesMatchingLibs )
# install( TARGETS matchClusters DESTINATION bin )


# cmessage( STATUS "Creating superimposeBackground app..." )
# add_executable( superimposeBackground ${CMAKE_CURRENT_SOURCE_DIR}/src/superimpose_signal_and_backgrounds.cpp )
# target_link_libraries( superimposeBackground planesMatchingLibs )
# install( TARGETS superimposeBackground DESTINATION bin )


