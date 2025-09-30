# plot_avg_times app
cmessage( STATUS "Creating plot_avg_times app..." )
add_executable( plot_avg_times ${CMAKE_CURRENT_SOURCE_DIR}/src/plot_avg_times.cpp )
target_link_libraries( plot_avg_times clusteringLibs)
install( TARGETS plot_avg_times DESTINATION bin )
cmessage( STATUS "Creating backtrack app..." )
add_executable( backtrack ${CMAKE_CURRENT_SOURCE_DIR}/src/backtrack.cpp )
target_link_libraries( backtrack clusteringLibs )
install( TARGETS backtrack DESTINATION bin )

cmessage( STATUS "Creating cluster app..." )
add_executable( cluster ${CMAKE_CURRENT_SOURCE_DIR}/src/cluster.cpp )
target_link_libraries( cluster clusteringLibs )
install( TARGETS cluster DESTINATION bin )

cmessage( STATUS "Creating analyze_tps app..." )
add_executable( analyze_tps ${CMAKE_CURRENT_SOURCE_DIR}/src/analyze_tps.cpp )
target_link_libraries( analyze_tps clusteringLibs )
install( TARGETS analyze_tps DESTINATION bin )

cmessage( STATUS "Creating analyze_clusters app..." )
add_executable( analyze_clusters ${CMAKE_CURRENT_SOURCE_DIR}/src/analyze_clusters.cpp )
target_link_libraries( analyze_clusters clusteringLibs )
install( TARGETS analyze_clusters DESTINATION bin )

cmessage( STATUS "Creating extract_calibration app..." )
add_executable( extract_calibration ${CMAKE_CURRENT_SOURCE_DIR}/src/extract_calibration.cpp )
target_link_libraries( extract_calibration clusteringLibs )
install( TARGETS extract_calibration DESTINATION bin )

cmessage( STATUS "Creating cluster_display app..." )
add_executable( cluster_display ${CMAKE_CURRENT_SOURCE_DIR}/src/cluster_display.cpp )
target_link_libraries( cluster_display clusteringLibs )
install( TARGETS cluster_display DESTINATION bin )

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
