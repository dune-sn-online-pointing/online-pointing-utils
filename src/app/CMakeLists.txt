# Applications

cmessage( STATUS "Creating plot_avg_times app..." )
add_executable( plot_avg_times ${CMAKE_CURRENT_SOURCE_DIR}/plot_avg_times.cpp )
target_link_libraries( plot_avg_times clustersLibs globalLib)
install( TARGETS plot_avg_times DESTINATION bin )

cmessage( STATUS "Creating make_clusters app..." )
add_executable( make_clusters ${CMAKE_CURRENT_SOURCE_DIR}/make_clusters.cpp )
target_link_libraries( make_clusters clustersLibs )
install( TARGETS make_clusters DESTINATION bin )

# cmessage( STATUS "Creating cluster_to_root app..." )
# add_executable( cluster_to_root ${CMAKE_CURRENT_SOURCE_DIR}/cluster_to_root.cpp )
# target_link_libraries( cluster_to_root clustersLibs )
# install( TARGETS cluster_to_root DESTINATION bin )

cmessage( STATUS "Creating backtrack_tpstream app..." )
add_executable( backtrack_tpstream ${CMAKE_CURRENT_SOURCE_DIR}/backtrack_tpstream.cpp )
target_link_libraries( backtrack_tpstream backtrackingLibs clustersLibs )
install( TARGETS backtrack_tpstream DESTINATION bin )

cmessage( STATUS "Creating analyze_clusters app..." )
add_executable( analyze_clusters ${CMAKE_CURRENT_SOURCE_DIR}/analyze_clusters.cpp )
target_link_libraries( analyze_clusters clustersLibs anaLib globalLib )
install( TARGETS analyze_clusters DESTINATION bin )

cmessage( STATUS "Creating analyze_tps app..." )
add_executable( analyze_tps ${CMAKE_CURRENT_SOURCE_DIR}/analyze_tps.cpp )
target_link_libraries( analyze_tps clustersLibs anaLib globalLib )
install( TARGETS analyze_tps DESTINATION bin )

cmessage( STATUS "Creating extract_calibration app..." )
add_executable( extract_calibration ${CMAKE_CURRENT_SOURCE_DIR}/extract_calibration.cpp )
target_link_libraries( extract_calibration clustersLibs globalLib )
install( TARGETS extract_calibration DESTINATION bin )

cmessage( STATUS "Creating display app..." )
add_executable( display ${CMAKE_CURRENT_SOURCE_DIR}/display.cpp )
target_link_libraries( display clustersLibs globalLib anaLib )
install( TARGETS display DESTINATION bin )

cmessage( STATUS "Creating add_backgrounds app..." )
add_executable( add_backgrounds ${CMAKE_CURRENT_SOURCE_DIR}/add_backgrounds.cpp )
target_link_libraries( add_backgrounds backtrackingLibs clustersLibs globalLib )
install( TARGETS add_backgrounds DESTINATION bin )

cmessage( STATUS "Creating extract_energy_cut_stats app..." )
add_executable( extract_energy_cut_stats ${CMAKE_CURRENT_SOURCE_DIR}/extract_energy_cut_stats.cpp )
target_link_libraries( extract_energy_cut_stats clustersLibs )
install( TARGETS extract_energy_cut_stats DESTINATION bin )

cmessage( STATUS "Creating split_by_apa app..." )
add_executable( split_by_apa ${CMAKE_CURRENT_SOURCE_DIR}/split_by_apa.cpp )
target_link_libraries( split_by_apa globalLib ${ROOT_LIBRARIES} )
install( TARGETS split_by_apa DESTINATION bin )

# Temporarily 
# cmessage( STATUS "Creating create_volume_clusters app..." )
# add_executable( create_volume_clusters ${CMAKE_CURRENT_SOURCE_DIR}/create_volume_clusters.cpp )
# target_link_libraries( create_volume_clusters clustersLibs )
# install( TARGETS create_volume_clusters DESTINATION bin )

cmessage( STATUS "Creating match_clusters app..." )
add_executable( match_clusters ${CMAKE_CURRENT_SOURCE_DIR}/match_clusters.cpp )
target_link_libraries( match_clusters clustersLibs )
install( TARGETS match_clusters DESTINATION bin )

cmessage( STATUS "Creating match_clusters_truth app..." )
add_executable( match_clusters_truth ${CMAKE_CURRENT_SOURCE_DIR}/match_clusters_truth.cpp )
target_link_libraries( match_clusters_truth clustersLibs globalLib )
install( TARGETS match_clusters_truth DESTINATION bin )

cmessage( STATUS "Creating diagnose_timing app..." )
add_executable( diagnose_timing ${CMAKE_CURRENT_SOURCE_DIR}/diagnose_timing.cpp )
target_link_libraries( diagnose_timing clustersLibs globalLib )
install( TARGETS diagnose_timing DESTINATION bin )

cmessage( STATUS "Creating analyze_matching app..." )
add_executable( analyze_matching ${CMAKE_CURRENT_SOURCE_DIR}/analyze_matching.cpp )
target_link_libraries( analyze_matching clustersLibs globalLib )
install( TARGETS analyze_matching DESTINATION bin )

# Temporarily 
# cmessage( STATUS "Creating superimpose_signal_and_backgrounds app..." )
# add_executable( superimpose_signal_and_backgrounds ${CMAKE_CURRENT_SOURCE_DIR}/superimpose_signal_and_backgrounds.cpp )
# target_link_libraries( superimpose_signal_and_backgrounds clustersLibs )
# install( TARGETS superimpose_signal_and_backgrounds DESTINATION bin )
